COMMENT
/**
 * @file HDF5reader.mod
 * @brief 
 * @author king
 * @date 2009-10-23
 * @remark Copyright © BBP/EPFL 2005-2011; All rights reserved. Do not distribute without further notice.
 */
ENDCOMMENT

COMMENT
This is intended to serve two purposes.  One as a general purpose reader for HDF5 files with a basic set of
accessor functions.  In addition, it will have special handling for our synapse data files such that they can be handled
more efficiently in a  massively parallel manner.  I feel inclined to spin this functionality out into a c++ class where
I can access more advanced coding structures, especially STL.
ENDCOMMENT

NEURON {
    THREADSAFE : have no evidence it is, but want to get bluron to accept
    ARTIFICIAL_CELL HDF5Reader
    BBCOREPOINTER ptr
}

VERBATIM
/* not quite sure what to do for bbcore. For now, nothing. */
static void bbcore_write(double* x, int* d, int* xx, int* dd, _threadargsproto_) {
  return;
}
static void bbcore_read(double* x, int* d, int* xx, int* dd, _threadargsproto_) {
  return;
}
ENDVERBATIM

PARAMETER {
}

ASSIGNED {
        ptr
}

INITIAL {
    closeFile()
}

NET_RECEIVE(w) {
}

VERBATIM

#undef ptr
#define H5_USE_16_API 1
#include "hdf5.h"
//#include "/opt/hdf5/include/hdf5.h"  //-linsrv
//#include "/users/delalond/Dev/bglib1.5/dep-install/include/hdf5.h" // Cray XT5
#include "mpi.h"
#include <stdlib.h>

/// NEURON utility functions we want to use
extern double* hoc_pgetarg(int iarg);
extern double* getarg(int iarg);
extern char* gargstr(int iarg);
extern int hoc_is_str_arg(int iarg);
extern int nrnmpi_numprocs;
extern int nrnmpi_myid;
extern int ifarg(int iarg);
extern double chkarg(int iarg, double low, double high);
extern double* vector_vec(void* vv);
extern int vector_capacity(void* vv);
extern void* vector_arg(int);
extern void* vector_resize();

/**
 * During synapse loading initialization, the h5 files with synapse data are catalogged
 * such that each cpu looks at a subset of what's available and gets ready to report to
 * other cpus where to find postsynaptic cells
 */
struct SynapseCatalog
{
    /// The base filename for synapse data.  The fileID is applied as a suffix (%s.%d)
    char* rootName;
    
    /// When working with multiple files, track the id of the active file open
    int fileID;
    
    /// The number of files this cpu has catalogged
    int nFiles;
    
    /// The IDs for the files this cpu has catalogged
    int *fileIDs;
    
    /// For each file, the number of gids catalogged
    int *availablegidCount;
    
    /// For each file, an array of the gids catalogged
    int **availablegids;
    
    /// The index of the gid being handled in the catalog
    int gidIndex;
};

typedef struct SynapseCatalog SynapseCatalog;



/**
 * After loading, the cpus will exchange requests for info about which files contain synapse
 * data for their local gids.  This is used to store the received info provided those gids receive
 */
struct ConfirmedCells
{
    /// The number of files and gids that are confirmed as loadable for this cpu
    int count;
    
    /// The gids that can be loaded by this cpu
    int *gids;
    
    /// The files to be opened for this cpu's gids
    int *fileIDs;
};

typedef struct ConfirmedCells ConfirmedCells;

/**
 * Hold persistent HDF5 data such as file handle and info about latest dataset loaded
 */
struct Info {
    
    /// data members for general HDF5 usage
    hid_t file_;
    float * datamatrix_;
    char name_group[256];
    hsize_t rowsize_;
    hsize_t columnsize_;
    
    /// Sometimes we want to silence certain warnings
    int verboseLevel;
    
    /// Used to catalog contents of some h5 files tracked by this cpu
    SynapseCatalog synapseCatalog;
    
    /// Receive info on which files contain data for gids local to this cpu
    ConfirmedCells confirmedCells;
};

typedef struct Info Info;

///Utility macro to make the NEURON pointer accessible for reading and writing (seems like we have more levels of indirection than necessary - JGK)
#define INFOCAST Info** ip = (Info**)(&(_p_ptr))

#define dp double*

/**
 * Utility function to ensure that all members of an Info struct are initialized.
 */
void initInfo( Info *info )
{
    // These fields are used when data is being accessed from a dataset
    info->file_ = -1;
    info->datamatrix_ = NULL;
    info->name_group[0] = '\0';
    info->rowsize_ = 0;
    info->columnsize_ = 0;
    
    info->verboseLevel = 1;
    
    // These fields are used exclusively for catalogging which h5 files contain which postsynaptic gids
    info->synapseCatalog.fileID = -1;
    info->synapseCatalog.fileIDs = NULL;
    info->synapseCatalog.nFiles = 0;
    info->synapseCatalog.availablegidCount = NULL;
    info->synapseCatalog.availablegids = NULL;
    info->synapseCatalog.gidIndex = 0;
    info->confirmedCells.count = 0;
    info->confirmedCells.gids = NULL;
    info->confirmedCells.fileIDs = NULL;
}



/**
 * Callback function for H5Giterate - if the dataset opened corresponds to a gid, it is catalogged so the
 * local cpu can inform other cpus the whereabouts of that gid
 *
 * @param loc_id hdf5 handle to the open file
 * @param name name of the dataset to be accessed during this iteration step
 * @param opdata not used since we have global Info object
 */
herr_t loadShareData( hid_t loc_id, const char *name, void *opdata )
{
    Info* info = (Info*)opdata;
    assert( info->file_ >= 0 );
    
    //fprintf( stderr, "open dataset %s\n", name );
    
    //make sure we are using a dataset that corresponds to a gid
    int gid = atoi( name+1 );
    char rebuild[32];
    snprintf( rebuild, 32, "a%d", gid );
    if( strcmp( rebuild, name ) != 0 ) {
        //non-synapse dataset, but not an error (could just be the version info)
        //fprintf( stderr, "ignore non-gid dataset\n" );
        return 0;
    }
    
    // we have confirmed that this dataset corresponds to a gid.  The active file should make it part of its data
    int fileIndex = info->synapseCatalog.nFiles-1;
    info->synapseCatalog.availablegids[ fileIndex ][ info->synapseCatalog.gidIndex++ ] = gid;
    
    return 1;
}



/**
 * Open an HDF5 file for reading.  In the event of synapse data, the datasets of the file may be iterated in order to
 * build a catalog of available gids and their file locations.
 *
 * @param info Structure that manages hdf5 info 
 * @param filename File to open
 * @param fileID Integer to identify this file (attached as suffix to filename)
 * @param nNodesPerFile 0: open file, but don't load data; 1: open file for catalogging; N: read portion of file for catalogging
 * @param startRank used to help calculate data range to load when file subportion is loaded
 * @param myRank used to help calculate data range to load when file subportion is loaded
 */
int openFile( Info* info, const char *filename, int fileID, int nRanksPerFile, int startRank, int myRank )
{
    if( info->file_ != -1 ) {
        H5Fclose(info->file_);
    }
    
    char nameoffile[512];
    //fprintf( stderr, "arg check: %s %d %d %d\n", filename, nRanksPerFile, startRank, myRank );
    if( fileID != -1 ) {
        snprintf( nameoffile, 512, "%s.%d", filename, fileID );
    } else {
        strncpy( nameoffile, filename, 512 );
    }
    
    info->file_ = H5Fopen( nameoffile, H5F_ACC_RDONLY, H5P_DEFAULT );
    info->name_group[0]='\0';
    if( info->file_ < 0 ) {
        info->file_ = -1;
        printf( "ERROR: Failed to open synapse file: %s\n", nameoffile );
        return -1;
    }
    
    if( nRanksPerFile == 0 ) {
        // don't need to load data yet, so we return
        return 0;
    }
    
    // will catalog synapse data
    info->synapseCatalog.fileID = fileID;
    
    int nDatasetsToImport=0, startIndex=0;
    hsize_t nObjects;
    H5Gget_num_objs( info->file_, &nObjects );
    
    if( nRanksPerFile == 1 ) {
        nDatasetsToImport = (int) nObjects;
    }
    else {
        // need to determine which indices to read
        nDatasetsToImport = (int) nObjects / nRanksPerFile;
        if( nObjects%nRanksPerFile != 0 )
            nDatasetsToImport++;
        
        startIndex = (myRank-startRank)*nDatasetsToImport;
        if( startIndex + nDatasetsToImport > (int) nObjects ) {
            nDatasetsToImport = (int) nObjects - startIndex;
            if( nDatasetsToImport <= 0 ) {
                //fprintf( stderr, "No need to import any data on rank %d since all %d are claimed\n", myRank, (int) nObjects );
                return 0;
            }
        }
    }
    
    int nFiles = ++info->synapseCatalog.nFiles;
    info->synapseCatalog.fileIDs = (int*) realloc ( info->synapseCatalog.fileIDs, sizeof(int)*nFiles );
    info->synapseCatalog.fileIDs[nFiles-1] = fileID;
    info->synapseCatalog.availablegidCount = (int*) realloc ( info->synapseCatalog.availablegidCount, sizeof(int)*nFiles );
    info->synapseCatalog.availablegids = (int**) realloc ( info->synapseCatalog.availablegids, sizeof(int*)*nFiles );
    
    info->synapseCatalog.availablegidCount[nFiles-1] = nDatasetsToImport;
    info->synapseCatalog.availablegids[nFiles-1] = (int*) calloc ( nDatasetsToImport, sizeof(int) );
    info->synapseCatalog.gidIndex=0;
    
    //fprintf( stderr, "load datasets %d through %d (max %d)\n", startIndex, startIndex+nDatasetsToImport, (int) nObjects );
    
    int i, verify=startIndex, result;
    for( i=startIndex; i<startIndex+nDatasetsToImport && i<nObjects; i++ ) {
        assert( verify == i );
        result = H5Giterate( info->file_, "/", &verify, loadShareData, info );
        if( result != 1 )
            continue;
    }
            
    return 0;
}



/**
 * Given the name of a dataset, load it from the current hdf5 file into the matrix pointer
 *
 * @param info Structure that manages hdf5 info, its datamatrix_ variable is populated with hdf5 data on success
 * @param name The name of the dataset to access and load in the hdf5 file
 */
int loadDataMatrix( Info *info, char* name )
{
    int isCurrentlyLoaded = strncmp( info->name_group, name, 256 ) == 0;
    if( isCurrentlyLoaded )
        return 0;
    
    hsize_t dims[2] = {0}, offset[2] = {0};
    hid_t dataset_id, dataspace;
    dataset_id = H5Dopen(info->file_, name);
    if( dataset_id < 0)
    {
        printf("Error accessing to dataset %s in synapse file\n", name);
        return -1;
    }
    
    strncpy(info->name_group, name, 256);
    
    dataspace = H5Dget_space(dataset_id);
    
    int dimensions = H5Sget_simple_extent_ndims(dataspace);
    //printf("Dimensions:%d\n",dimensions);
    H5Sget_simple_extent_dims(dataspace,dims,NULL);
    //printf("Accessing to %s , nrow:%lu,ncolumns:%lu\n",info->name_group,(unsigned long)dims[0],(unsigned long)dims[1]);
    info->rowsize_ = (unsigned long)dims[0];
    if( dimensions > 1 )
        info->columnsize_ = dims[1];
    else
        info->columnsize_ = 1;
    //printf("\n Size of data is row= [%d], Col = [%lu]\n", dims[0], (unsigned long)dims[1]);
    if(info->datamatrix_ != NULL)
    {
        //printf("Freeeing memory \n ");
        free(info->datamatrix_);
    }
    info->datamatrix_ = (float *) malloc(sizeof(float) *(info->rowsize_*info->columnsize_)); 
    //info->datamatrix_ = (float *) hoc_Emalloc(sizeof(float) *(info->rowsize_*info->columnsize_)); hoc_malchk();
    // Last line fails, corrupt memory of argument 1 and  probably more
    H5Sselect_hyperslab(dataspace, H5S_SELECT_SET, offset, NULL, dims, NULL);
    hid_t dataspacetogetdata=H5Screate_simple(dimensions,dims,NULL);
    H5Dread(dataset_id,H5T_NATIVE_FLOAT,dataspacetogetdata,H5S_ALL,H5P_DEFAULT,info->datamatrix_);
    H5Sclose(dataspace);
    H5Sclose(dataspacetogetdata);
    H5Dclose(dataset_id);
    //printf("Working , accessed %s , on argstr1 %s\n",info->name_group,gargstr(1));
    return 0;
}

ENDVERBATIM



: cannot translate CONSTRUCTOR as vectorized threadsafe method
:CONSTRUCTOR { : double - loc of point process ??? ,string filename
PROCEDURE construct() {
VERBATIM {
    char nameoffile[512];
    int nFiles = 1;
    
    if( ifarg(2) ) {
        nFiles = *getarg(2);
    }
        
    if(ifarg(1) && hoc_is_str_arg(1)) {
        INFOCAST;
        Info* info = 0;
        
        strncpy(nameoffile, gargstr(1),256);
        info = (Info*) hoc_Emalloc(sizeof(Info)); hoc_malchk();
        initInfo( info );
        
        if( ifarg(3) ) {
            info->verboseLevel = *getarg(3);
        }
        
        *ip = info;
        
        if( nFiles == 1 ) {
            // normal case - open a file and be ready to load data as needed
            openFile( info, nameoffile, -1, 0, 0, 0 );
        }
        else
        {
            // Each cpu is reponsible for a portion of the data
            info->synapseCatalog.rootName = strdup( nameoffile );
            
            int mpi_size, mpi_rank;
            MPI_Comm_size( MPI_COMM_WORLD, &mpi_size );
            MPI_Comm_rank( MPI_COMM_WORLD, &mpi_rank );
            
            //fprintf( stderr, "%d vs %d\n", mpi_size, nFiles );
            
            // need to determine if I open multiple files per cpu or multiple cpus share a file
            if( mpi_size > nFiles ) { // multiple cpus per file
                int nRanksPerFile = (int) mpi_size / nFiles;
                if( mpi_size % nFiles != 0 )
                    nRanksPerFile++;
                    
                //fprintf( stderr, "nRanksPerFile %d = %d/%d\n", nRanksPerFile, mpi_size, nFiles );
                
                if( nRanksPerFile * nFiles > mpi_size ) { // no files left for this rank
                    info->file_ = -1;
                    return 0;
                }
                
                int fileIndex = (int) mpi_rank / nRanksPerFile;  //this should be truncated
                int startRank = fileIndex * nRanksPerFile;
                
                //sprintf( nameoffile, "%s.%d", gargstr(1), fileIndex );
                //fprintf( stderr, "I should open file %s\n", nameoffile );
                
                openFile( info, nameoffile, fileIndex, nRanksPerFile, startRank, mpi_rank );
                
            } else {
                // one or more files per cpu - any file opened should load all the data.
                int nFilesPerRank = nFiles / mpi_size;
                //fprintf( stderr, "nFilesPerRank %d = %d/%d\n", nFilesPerRank, nFiles, mpi_size );
                
                if( nFiles % mpi_size != 0 ) {
                    nFilesPerRank++;
                }
                int startFile = mpi_rank * nFilesPerRank;
                if( startFile+nFilesPerRank > nFiles ) {
                    nFilesPerRank = nFiles-startFile;
                    if( nFilesPerRank <= 0 ) {
                        info->file_ = -1;
                        return 0;
                    }
                }
                
                int fileIndex=0;
                for( fileIndex=0; fileIndex<nFilesPerRank; fileIndex++ ) {
                    //sprintf( nameoffile, "%s.%d", gargstr(1), startFile+fileIndex );
                    openFile( info, nameoffile, startFile+fileIndex, 1, 0, 0 );
                }
            }
        }
    }
}
ENDVERBATIM
}



: cannot translate DESTRUCTOR as vectorized threadsafe method
:DESTRUCTOR {
PROCEDURE destruct() {
VERBATIM { 
    INFOCAST; Info* info = *ip; 
    if(info->file_>=0)
    {
        //printf("Trying to close\n");
        H5Fclose(info->file_);
        //printf("Close\n");
        info->file_ = -1;
    }
    if(info->datamatrix_ != NULL)
    {
        free(info->datamatrix_);
        info->datamatrix_ = NULL;
    }
}
ENDVERBATIM
}



FUNCTION redirect() {
VERBATIM {
    FILE *fout;
    char fname[128];
    
    int mpi_size, mpi_rank;
    //fprintf( stderr, "rank %d\n", getpid() );
    //sleep(20);

    
    // get MPI info
    MPI_Comm_size (MPI_COMM_WORLD, &mpi_size);
    MPI_Comm_rank (MPI_COMM_WORLD, &mpi_rank);  
    
    if( mpi_rank != 0 ) {    
        sprintf( fname, "NodeFiles/%d.%dnode.out", mpi_rank, mpi_size );
        fout = freopen( fname, "w", stdout );
        if( !fout ) {
            fprintf( stderr, "failed to redirect.  Terminating\n" );
            exit(0);
        }
        
        sprintf( fname, "NodeFiles/%d.%dnode.err", mpi_rank, mpi_size );
        fout = freopen( fname, "w", stderr );
        setbuf( fout, NULL );
    }
}
ENDVERBATIM
}

FUNCTION checkVersion() {
VERBATIM {
    INFOCAST; 
    Info* info = *ip;
    int mpi_size, mpi_rank;

    // get MPI info
    MPI_Comm_size (MPI_COMM_WORLD, &mpi_size);
    MPI_Comm_rank (MPI_COMM_WORLD, &mpi_rank);  
    
    int versionNumber = 0;
    
    //check version for synapse file; must be version 1 -> only have processor 0 do this to avoid output overload on errors
    if( mpi_rank == 0 )
    {
        hid_t dataset_id = H5Dopen( info->file_, "version" );
        if( dataset_id < 0 ) //no version info - must be version 0
        {
            fprintf( stderr, "Error. Incompatible synapse version file (given version 0 file, require version 1).\n" );
            fprintf( stderr, "Terminating" );
            MPI_Abort( MPI_COMM_WORLD, 27 );
        }
        
        H5Aread( H5Aopen_name( dataset_id, "attr" ), H5T_NATIVE_INT, &versionNumber );
        if( versionNumber != 1 )
        {
            fprintf( stderr, "Error. Incompatible synapse version file (given version %d file, require version 1).\n", versionNumber );
            fprintf( stderr, "Terminating" );
            MPI_Abort( MPI_COMM_WORLD, 28 );
        }
        
        H5Dclose(dataset_id);
    }
    return 0;
}
ENDVERBATIM
}

FUNCTION loadData() {
VERBATIM { 
    INFOCAST; 
    Info* info = *ip;
    
    if(info->file_>=0 && ifarg(1) && hoc_is_str_arg(1))
    {
        return loadDataMatrix( info, gargstr(1) );
    }
    else if( ifarg(1) )
    {
        int gid = *getarg(1);
        int gidIndex=0;
        
        for( gidIndex=0; gidIndex<info->confirmedCells.count; gidIndex++ ) {
            if( info->confirmedCells.gids[gidIndex] == gid ) {
                openFile( info, info->synapseCatalog.rootName, info->confirmedCells.fileIDs[gidIndex], 0, 0, 0 );
                
                char cellname[256];
                sprintf( cellname, "a%d", gid );
                
                return loadDataMatrix( info, cellname );
            }
        }
        
        //if we reach here, did not find data
        if( info->verboseLevel > 0 )
            fprintf( stderr, "Warning: failed to find data for gid %d\n", gid );
    }
    
    return 0;
}
ENDVERBATIM
}



FUNCTION getNoOfColumns(){ : string cellname
VERBATIM { 
    INFOCAST;
    Info* info = *ip;
    //printf("(Inside number of Col)Number of Col %s\n",gargstr(1));
    if(info->file_>=0 && ifarg(1) && hoc_is_str_arg(1))
    {
        char name[256];
        strncpy(name,gargstr(1),256);
        if(strncmp(info->name_group,name,256) == 0)
        {
            //printf("Returning :%d\n",(int)info->rowsize_);
            int res = (int) info->columnsize_;
            //printf("Res :%d\n",res);
            return res;
        }
        //printf("NumberofCol Error on the name of last loaded data: trying to access:%s loaded:%s\n",name,info->name_group);
        return 0;
    }
    else
    {
        return 0;
    }
}
ENDVERBATIM
}       



FUNCTION numberofrows() { : string cellname
VERBATIM { 
    INFOCAST; 
    Info* info = *ip;
    //printf("(Inside number of rows)Number of rows %s\n",gargstr(1));
    if(info->file_>=0 && ifarg(1) && hoc_is_str_arg(1))
    {
        char name[256];
        strncpy(name,gargstr(1),256);
        if(strncmp(info->name_group,name,256) == 0)
        {
            //printf("Returning :%d\n",(int)info->rowsize_);
            int res = (int) info->rowsize_;
            //printf("Res :%d\n",res);
            return res;  
        }
        //printf("Numberofrows Error on the name of last loaded data: trying to access:%s loaded:%s\n",name,info->name_group);
        return 0;
    }
    else
    {
        return 0;
    }
}
ENDVERBATIM
}



FUNCTION getData() {
VERBATIM { 
    INFOCAST; 
    Info* info = *ip;
    if(info->file_>=0&& ifarg(1) && hoc_is_str_arg(1) && ifarg(2) && ifarg(3))
    {
        char name[256];
        strncpy(name,gargstr(1),256);
        if(strncmp(info->name_group,name,256) == 0)
        {
            hsize_t row,column;
            row = (hsize_t) *getarg(2);
            column = (hsize_t) *getarg(3);
            if(row<0 || row >=info->rowsize_ || column < 0 || column>=info->columnsize_)
            {
                printf("ERROR: trying to access to a row and column erroneus on %s, size: %d,%d accessing to %d,%d\n ",name,info->rowsize_,info->columnsize_,row,column);
                return 0;
            }
            float res = info->datamatrix_[row*info->columnsize_ + column];
            return res;
        }
        printf("(Getting data)Error on the name of last loaded data: access:%s loaded:%s\n",name,info->name_group);
        return 0;
    }
    else
    {
        //printf("ERROR:Error on number of rows of \n");
        return 0;
    }
}
ENDVERBATIM
}



FUNCTION getColumnDataRange() {
VERBATIM { 
    INFOCAST; 
    Info* info = *ip;
    void* pdVec = NULL;
    double* pd  = NULL;
    int i = 0;
    int nStart, nEnd, count;
    if(info->file_>=0&& ifarg(1) && hoc_is_str_arg(1) && ifarg(2) )
    {
        char name[256];
        strncpy(name,gargstr(1),256);
        if(strncmp(info->name_group,name,256) == 0)
        {
            hsize_t column;
            column  = (hsize_t) *getarg(2);
            if(column<0 || column >=info->columnsize_ )
            {
                printf("ERROR: trying to access to a column erroneus on %s, size: %d,%d accessing to column %d\n ",name,info->rowsize_,info->columnsize_,column);
                return 0;
            }
            pdVec = vector_arg(3);
            nStart = (int)*getarg(4);
            nEnd  = (int)*getarg(5);
            vector_resize(pdVec, nEnd-nStart+1);
            pd = vector_vec(pdVec);
            count =0;
            for( i=nStart; i<=nEnd; i++){
                pd[count] = info->datamatrix_[i*info->columnsize_ + column];
                count = count +1;
                //printf("\n Filling [%f]", pd[i]);
            }
            //float res = info->datamatrix_[row*info->columnsize_ + column];
            return 1;
        }
        printf("(Getting data)Error on the name of last loaded data: access:%s loaded:%s\n",name,info->name_group);
        return 0;
    }
    else
    {
        //printf("ERROR:Error on number of rows of \n");
        return 0;
    } 
}
ENDVERBATIM
}



COMMENT
/**
 * Load all the data from a single column of active dataset into a NEURON Vector object
 *
 * @param dataset name - used to confirm that the active dataset matches what is requested
 * @param column index
 * @param Vector object to fill - resized as needed to hold the available data
 */
ENDCOMMENT
FUNCTION getColumnData() {
VERBATIM { 
    INFOCAST; 
    Info* info = *ip;
    void* pdVec = NULL;
    double* pd  = NULL;
    int i = 0;
    if(info->file_>=0&& ifarg(1) && hoc_is_str_arg(1) && ifarg(2) )
    {
        char name[256];
        strncpy(name,gargstr(1),256);
        if(strncmp(info->name_group,name,256) == 0)
        {
            hsize_t column;
            column  = (hsize_t) *getarg(2);
            if(column<0 || column >=info->columnsize_ )
            {
                printf("ERROR: trying to access to a column erroneus on %s, size: %d,%d accessing to column %d\n ",name,info->rowsize_,info->columnsize_,column);
                return 0;
            }
            pdVec = vector_arg(3);
            vector_resize(pdVec, (int) info->rowsize_);
            pd = vector_vec(pdVec);
            for( i=0; i<info->rowsize_; i++){
                pd[i] = info->datamatrix_[i*info->columnsize_ + column];
                //printf("\n Filling [%f]", pd[i]);
            }
            //float res = info->datamatrix_[row*info->columnsize_ + column];
            return 1;
        }
        printf("(Getting data)Error on the name of last loaded data: access:%s loaded:%s\n",name,info->name_group);
        return 0;
    }
    else
    {
        //printf("ERROR:Error on number of rows of \n");
        return 0;
    } 
}
ENDVERBATIM
}



COMMENT
/**
 * Retrieve the value for an attribute of the active dataset.  Expected to contain only one value of double type
 * 
 * @param dataset name
 * @param attribute name
 */
ENDCOMMENT
FUNCTION getAttributeValue() {
VERBATIM
    INFOCAST; 
    Info* info = *ip;
    if( info->file_ >= 0 && ifarg(1) && hoc_is_str_arg(1) && ifarg(2) && hoc_is_str_arg(2) )
    {
        hid_t dataset_id = H5Dopen( info->file_, gargstr(1) );
        if( dataset_id < 0 )
        {
            fprintf( stderr, "Error: no dataset with name %s available.\n", gargstr(1) );
            return 0;
        }
        
        double soughtValue;
        hid_t attr_id = H5Aopen_name( dataset_id, gargstr(2) );
        if( attr_id < 0 ) {
            fprintf( stderr, "Error: failed to open attribute %s\n", gargstr(2) );
            return 0;
        }
        H5Aread( attr_id, H5T_NATIVE_DOUBLE, &soughtValue );
        H5Dclose(dataset_id);
        
        return soughtValue;
    }
    
    return 0;
ENDVERBATIM
}



FUNCTION closeFile() {
VERBATIM { 
    INFOCAST; 
    Info* info = *ip;
    if (!info) { return 0.0; }
    if(info->file_ >=0)
    {
        //printf("Trying to close\n");
        H5Fclose(info->file_);
        //printf("Close\n");
        info->file_ = -1;
    }
    if(info->datamatrix_ != NULL)
    {
        free(info->datamatrix_);
        info->datamatrix_ = NULL;
    }
}
ENDVERBATIM
}



COMMENT
/**
 * This function will have each cpu learn from the other cpus which nrn.h5 file contains the data it needs.  It can then load
 * the data as needed using the older functions.  Hopefully this will be better since it avoids loading data that is not needed and then
 * transmitting it across the network.  My concern is that there will still be some contention on the various nrn.h5 files with multiple cpus
 * accessing it at once, but we will see how that unfolds.
 *
 * @param gidvec hoc vector containing list of gids on local cpu
 */
ENDCOMMENT
FUNCTION exchangeSynapseLocations() {
VERBATIM
    INFOCAST; 
    Info* info = *ip;
    
    int mpi_size, mpi_rank;
    MPI_Comm_size (MPI_COMM_WORLD, &mpi_size);
    MPI_Comm_rank (MPI_COMM_WORLD, &mpi_rank);
    
    void* vv = vector_arg(1);
    int gidCount = vector_capacity(vv);
    double *gidList = vector_vec(vv);
    
    // used to store the number of gids found per cpu
    int *foundCountsAcrossCPUs = (int*) malloc( sizeof(int)*mpi_size );
    int *foundDispls = (int*) malloc( sizeof(int)*mpi_size );
    
    int bufferSize;
    
    // have every cpu allocate a buffer capable of holding the data for the cpu with the most cells
    MPI_Allreduce( &gidCount, &bufferSize, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD );
    double* gidsRequested = (double*) malloc ( sizeof(double)*bufferSize );
    int* gidsFound = (int*) malloc( sizeof(int)*bufferSize );
    int* fileIDsFound = (int*) malloc( sizeof(int)*bufferSize );
    
    double *tempRequest;  //used to hold the gidsRequested buffer when this cpu is broadcasting its gidList
    
    // each cpu in turn will bcast its local gids.  It should then get back the file indices where the data can be found.
    int activeRank, requestCount;
    for( activeRank=0; activeRank<mpi_size; activeRank++ ) {
        if( activeRank == mpi_rank ) {
            requestCount = gidCount;
            tempRequest = gidsRequested;
            gidsRequested = gidList;
        }
        
        MPI_Bcast( &requestCount, 1, MPI_INT, activeRank, MPI_COMM_WORLD );
        MPI_Bcast( gidsRequested, requestCount, MPI_DOUBLE, activeRank, MPI_COMM_WORLD );
                
        // each cpu will check if the requested gids exist in its list
        int nFiles = 0;
        
        // TODO: linear searches at the moment.  Maybe a more efficient ln(n) search.  Although, I would prefer to implement that
        //  with access to STL or the like data structures.
        int fileIndex, gidIndex, requestIndex;
        for( fileIndex=0; fileIndex < info->synapseCatalog.nFiles; fileIndex++ ) {
            for( gidIndex=0; gidIndex < info->synapseCatalog.availablegidCount[fileIndex]; gidIndex++ ) {
                for( requestIndex=0; requestIndex < requestCount; requestIndex++ ) {
                    if( info->synapseCatalog.availablegids[fileIndex][gidIndex] == gidsRequested[requestIndex] ) {
                        gidsFound[nFiles] = gidsRequested[requestIndex];
                        fileIDsFound[nFiles++] = info->synapseCatalog.fileIDs[fileIndex];
                    }
                }
            }
        }
        
        MPI_Gather( &nFiles, 1, MPI_INT, foundCountsAcrossCPUs, 1, MPI_INT, activeRank, MPI_COMM_WORLD );
        
        if( activeRank == mpi_rank ) {
            info->confirmedCells.count = 0;
            
            int nodeIndex;
            for( nodeIndex=0; nodeIndex<mpi_size; nodeIndex++ ) {
                foundDispls[nodeIndex] = info->confirmedCells.count;
                info->confirmedCells.count += foundCountsAcrossCPUs[nodeIndex];
            }
            info->confirmedCells.gids = (int*) malloc ( sizeof(int)*info->confirmedCells.count );
            info->confirmedCells.fileIDs = (int*) malloc ( sizeof(int)*info->confirmedCells.count );
        }
        
        MPI_Gatherv( gidsFound, nFiles, MPI_INT, info->confirmedCells.gids, foundCountsAcrossCPUs, foundDispls, MPI_INT, activeRank, MPI_COMM_WORLD );
        MPI_Gatherv( fileIDsFound, nFiles, MPI_INT, info->confirmedCells.fileIDs, foundCountsAcrossCPUs, foundDispls, MPI_INT, activeRank, MPI_COMM_WORLD );
        
        // put back the original gid request buffer so as to not destroy this cpu's gidList
        if( activeRank == mpi_rank ) {
            gidsRequested = tempRequest;
        }

    }

    free(gidsRequested);
    free(gidsFound);
    free(fileIDsFound);
    free(foundCountsAcrossCPUs);
    free(foundDispls);
    
ENDVERBATIM
}
