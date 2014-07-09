/* Created by Language version: 6.2.0 */
/* VECTORIZED */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "corebluron/mech/cfile/scoplib.h"
#undef PI
 
#include "corebluron/nrnoc/md1redef.h"
#include "corebluron/nrnconf.h"
#include "corebluron/nrnoc/multicore.h"

#include "corebluron/nrnoc/md2redef.h"
#if METHOD3
extern int _method3;
#endif

#undef exp
#define exp hoc_Exp
extern double hoc_Exp();
 
#define _threadargscomma_ _p, _ppvar, _thread, _nt,
#define _threadargs_ _p, _ppvar, _thread, _nt
#define _threadargsproto_ double* _p, Datum* _ppvar, ThreadDatum* _thread, _NrnThread* _nt
 	/*SUPPRESS 761*/
	/*SUPPRESS 762*/
	/*SUPPRESS 763*/
	/*SUPPRESS 765*/
	 extern double *getarg();
 /* Thread safe. No static _p or _ppvar. */
 
#define t _nt->_t
#define dt _nt->_dt
#define v _p[0]
#define _tsav _p[1]
#define _nd_area  _nt->_data[_ppvar[0]]
#define _p_ptr	_nt->_vdata[_ppvar[2]]
 
#if MAC
#if !defined(v)
#define v _mlhv
#endif
#if !defined(h)
#define h _mlhh
#endif
#endif
 static int hoc_nrnpointerindex =  2;
 static ThreadDatum* _extcall_thread;
 /* external NEURON variables */
 
#if 0 /*BBCORE*/
 /* declaration of user functions */
 static double _hoc_checkVersion();
 static double _hoc_construct();
 static double _hoc_closeFile();
 static double _hoc_destruct();
 static double _hoc_exchangeSynapseLocations();
 static double _hoc_getAttributeValue();
 static double _hoc_getColumnData();
 static double _hoc_getColumnDataRange();
 static double _hoc_getData();
 static double _hoc_getNoOfColumns();
 static double _hoc_loadData();
 static double _hoc_numberofrows();
 static double _hoc_redirect();
 
#endif /*BBCORE*/
 static int _mechtype;
extern int nrn_get_mechtype();
 static int _pointtype;
 
#if 0 /*BBCORE*/
 static void* _hoc_create_pnt(_ho) Object* _ho; { void* create_point_process();
 return create_point_process(_pointtype, _ho);
}
 static void _hoc_destroy_pnt();
 static double _hoc_loc_pnt(_vptr) void* _vptr; {double loc_point_process();
 return loc_point_process(_pointtype, _vptr);
}
 static double _hoc_has_loc(_vptr) void* _vptr; {double has_loc_point();
 return has_loc_point(_vptr);
}
 static double _hoc_get_loc_pnt(_vptr)void* _vptr; {
 double get_loc_point_process(); return (get_loc_point_process(_vptr));
}
 
#endif /*BBCORE*/
 
#if 0 /*BBCORE*/
 /* connect user functions to hoc names */
 static IntFunc hoc_intfunc[] = {
 0,0
};
 static struct Member_func {
	char* _name; double (*_member)();} _member_func[] = {
 "loc", _hoc_loc_pnt,
 "has_loc", _hoc_has_loc,
 "get_loc", _hoc_get_loc_pnt,
 "checkVersion", _hoc_checkVersion,
 "construct", _hoc_construct,
 "closeFile", _hoc_closeFile,
 "destruct", _hoc_destruct,
 "exchangeSynapseLocations", _hoc_exchangeSynapseLocations,
 "getAttributeValue", _hoc_getAttributeValue,
 "getColumnData", _hoc_getColumnData,
 "getColumnDataRange", _hoc_getColumnDataRange,
 "getData", _hoc_getData,
 "getNoOfColumns", _hoc_getNoOfColumns,
 "loadData", _hoc_loadData,
 "numberofrows", _hoc_numberofrows,
 "redirect", _hoc_redirect,
 0, 0
};
 
#endif /*BBCORE*/
#define checkVersion checkVersion_HDF5Reader
#define closeFile closeFile_HDF5Reader
#define exchangeSynapseLocations exchangeSynapseLocations_HDF5Reader
#define getAttributeValue getAttributeValue_HDF5Reader
#define getColumnData getColumnData_HDF5Reader
#define getColumnDataRange getColumnDataRange_HDF5Reader
#define getData getData_HDF5Reader
#define getNoOfColumns getNoOfColumns_HDF5Reader
#define loadData loadData_HDF5Reader
#define numberofrows numberofrows_HDF5Reader
#define redirect redirect_HDF5Reader
 extern double checkVersion();
 extern double closeFile();
 extern double exchangeSynapseLocations();
 extern double getAttributeValue();
 extern double getColumnData();
 extern double getColumnDataRange();
 extern double getData();
 extern double getNoOfColumns();
 extern double loadData();
 extern double numberofrows();
 extern double redirect();
 /* declare global and static user variables */
 
#if 0 /*BBCORE*/
 /* some parameters have upper and lower limits */
 static HocParmLimits _hoc_parm_limits[] = {
 0,0,0
};
 static HocParmUnits _hoc_parm_units[] = {
 0,0
};
 
#endif /*BBCORE*/
 
#if 0 /*BBCORE*/
 /* connect global user variables to hoc */
 static DoubScal hoc_scdoub[] = {
 0,0
};
 static DoubVec hoc_vdoub[] = {
 0,0,0
};
 
#endif /*BBCORE*/
 static double _sav_indep;
 static void nrn_alloc(), nrn_init(), nrn_state();
 
#if 0 /*BBCORE*/
 static void _hoc_destroy_pnt(_vptr) void* _vptr; {
   destroy_point_process(_vptr);
}
 
#endif /*BBCORE*/
 /* connect range variables in _p that hoc is supposed to know about */
 static const char *_mechanism[] = {
 "6.2.0",
"HDF5Reader",
 0,
 0,
 0,
 "ptr",
 0};
 
static void nrn_alloc(double* _p, Datum* _ppvar, int _type) {
 	/*initialize range parameters*/
 
#if 0 /*BBCORE*/
 
#endif /* BBCORE */
 
}
 static _initlists();
 static _net_receive();
 typedef (*_Pfrv)();
 extern _Pfrv* pnt_receive;
 extern short* pnt_receive_size;
 
#define _psize 2
#define _ppsize 3
 static void bbcore_read(double *, int*, int*, int*, _threadargsproto_);
 extern void hoc_reg_bbcore_read(int, void(*)(double *, int*, int*, int*, _threadargsproto_));
 _HDF5reader_reg() {
	int _vectorized = 1;
  _initlists();
 
#if 0 /*BBCORE*/
 
#endif /*BBCORE*/
 	_pointtype = point_register_mech(_mechanism,
	 nrn_alloc,(void*)0, (void*)0, (void*)0, nrn_init,
	 hoc_nrnpointerindex,
	 NULL/*_hoc_create_pnt*/, NULL/*_hoc_destroy_pnt*/, /*_member_func,*/
	 1);
 _mechtype = nrn_get_mechtype(_mechanism[1]);
   hoc_reg_bbcore_read(_mechtype, bbcore_read);
  hoc_register_prop_size(_mechtype, _psize, _ppsize);
 add_nrn_artcell(_mechtype, 0);
 pnt_receive[_mechtype] = _net_receive;
 pnt_receive_size[_mechtype] = 1;
 }
static int _reset;
static char *modelname = "";

static int error;
static int _ninits = 0;
static int _match_recurse=1;
static _modl_cleanup(){ _match_recurse=1;}
static construct();
static destruct();
 
/*VERBATIM*/
/* not quite sure what to do for bbcore. For now, nothing. */
static void bbcore_write(double* x, int* d, int* xx, int* dd, _threadargsproto_) {
  return;
}
static void bbcore_read(double* x, int* d, int* xx, int* dd, _threadargsproto_) {
  return;
}
 
static _net_receive (_pnt, _args, _lflag) Point_process* _pnt; double* _args; double _lflag; 
{  double* _p; Datum* _ppvar; ThreadDatum* _thread; _NrnThread* _nt;
   _thread = (ThreadDatum*)0; _nt = (_NrnThread*)_pnt->_vnt;   _p = _pnt->_data; _ppvar = _pnt->_pdata;
  assert(_tsav <= t); _tsav = t; {
   } }
 
/*VERBATIM*/

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

 
static int  construct ( _p, _ppvar, _thread, _nt ) double* _p; Datum* _ppvar; ThreadDatum* _thread; _NrnThread* _nt; {
   
/*VERBATIM*/
{
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
  return 0; }
 
#if 0 /*BBCORE*/
 
static double _hoc_construct(_vptr) void* _vptr; {
 double _r;
   double* _p; Datum* _ppvar; ThreadDatum* _thread; _NrnThread* _nt;
   _p = ((Point_process*)_vptr)->_prop->param;
  _ppvar = ((Point_process*)_vptr)->_prop->dparam;
  _thread = _extcall_thread;
  _nt = (_NrnThread*)((Point_process*)_vptr)->_vnt;
 _r = 1.;
 construct ( _p, _ppvar, _thread, _nt ) ;
 return(_r);
}
 
#endif /*BBCORE*/
 
static int  destruct ( _p, _ppvar, _thread, _nt ) double* _p; Datum* _ppvar; ThreadDatum* _thread; _NrnThread* _nt; {
   
/*VERBATIM*/
{ 
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
  return 0; }
 
#if 0 /*BBCORE*/
 
static double _hoc_destruct(_vptr) void* _vptr; {
 double _r;
   double* _p; Datum* _ppvar; ThreadDatum* _thread; _NrnThread* _nt;
   _p = ((Point_process*)_vptr)->_prop->param;
  _ppvar = ((Point_process*)_vptr)->_prop->dparam;
  _thread = _extcall_thread;
  _nt = (_NrnThread*)((Point_process*)_vptr)->_vnt;
 _r = 1.;
 destruct ( _p, _ppvar, _thread, _nt ) ;
 return(_r);
}
 
#endif /*BBCORE*/
 
double redirect ( _p, _ppvar, _thread, _nt ) double* _p; Datum* _ppvar; ThreadDatum* _thread; _NrnThread* _nt; {
   double _lredirect;
 
/*VERBATIM*/
{
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
 
return _lredirect;
 }
 
#if 0 /*BBCORE*/
 
static double _hoc_redirect(_vptr) void* _vptr; {
 double _r;
   double* _p; Datum* _ppvar; ThreadDatum* _thread; _NrnThread* _nt;
   _p = ((Point_process*)_vptr)->_prop->param;
  _ppvar = ((Point_process*)_vptr)->_prop->dparam;
  _thread = _extcall_thread;
  _nt = (_NrnThread*)((Point_process*)_vptr)->_vnt;
 _r =  redirect ( _p, _ppvar, _thread, _nt ) ;
 return(_r);
}
 
#endif /*BBCORE*/
 
double checkVersion ( _p, _ppvar, _thread, _nt ) double* _p; Datum* _ppvar; ThreadDatum* _thread; _NrnThread* _nt; {
   double _lcheckVersion;
 
/*VERBATIM*/
{
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
 
return _lcheckVersion;
 }
 
#if 0 /*BBCORE*/
 
static double _hoc_checkVersion(_vptr) void* _vptr; {
 double _r;
   double* _p; Datum* _ppvar; ThreadDatum* _thread; _NrnThread* _nt;
   _p = ((Point_process*)_vptr)->_prop->param;
  _ppvar = ((Point_process*)_vptr)->_prop->dparam;
  _thread = _extcall_thread;
  _nt = (_NrnThread*)((Point_process*)_vptr)->_vnt;
 _r =  checkVersion ( _p, _ppvar, _thread, _nt ) ;
 return(_r);
}
 
#endif /*BBCORE*/
 
double loadData ( _p, _ppvar, _thread, _nt ) double* _p; Datum* _ppvar; ThreadDatum* _thread; _NrnThread* _nt; {
   double _lloadData;
 
/*VERBATIM*/
{ 
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
 
return _lloadData;
 }
 
#if 0 /*BBCORE*/
 
static double _hoc_loadData(_vptr) void* _vptr; {
 double _r;
   double* _p; Datum* _ppvar; ThreadDatum* _thread; _NrnThread* _nt;
   _p = ((Point_process*)_vptr)->_prop->param;
  _ppvar = ((Point_process*)_vptr)->_prop->dparam;
  _thread = _extcall_thread;
  _nt = (_NrnThread*)((Point_process*)_vptr)->_vnt;
 _r =  loadData ( _p, _ppvar, _thread, _nt ) ;
 return(_r);
}
 
#endif /*BBCORE*/
 
double getNoOfColumns ( _p, _ppvar, _thread, _nt ) double* _p; Datum* _ppvar; ThreadDatum* _thread; _NrnThread* _nt; {
   double _lgetNoOfColumns;
 
/*VERBATIM*/
{ 
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
 
return _lgetNoOfColumns;
 }
 
#if 0 /*BBCORE*/
 
static double _hoc_getNoOfColumns(_vptr) void* _vptr; {
 double _r;
   double* _p; Datum* _ppvar; ThreadDatum* _thread; _NrnThread* _nt;
   _p = ((Point_process*)_vptr)->_prop->param;
  _ppvar = ((Point_process*)_vptr)->_prop->dparam;
  _thread = _extcall_thread;
  _nt = (_NrnThread*)((Point_process*)_vptr)->_vnt;
 _r =  getNoOfColumns ( _p, _ppvar, _thread, _nt ) ;
 return(_r);
}
 
#endif /*BBCORE*/
 
double numberofrows ( _p, _ppvar, _thread, _nt ) double* _p; Datum* _ppvar; ThreadDatum* _thread; _NrnThread* _nt; {
   double _lnumberofrows;
 
/*VERBATIM*/
{ 
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
 
return _lnumberofrows;
 }
 
#if 0 /*BBCORE*/
 
static double _hoc_numberofrows(_vptr) void* _vptr; {
 double _r;
   double* _p; Datum* _ppvar; ThreadDatum* _thread; _NrnThread* _nt;
   _p = ((Point_process*)_vptr)->_prop->param;
  _ppvar = ((Point_process*)_vptr)->_prop->dparam;
  _thread = _extcall_thread;
  _nt = (_NrnThread*)((Point_process*)_vptr)->_vnt;
 _r =  numberofrows ( _p, _ppvar, _thread, _nt ) ;
 return(_r);
}
 
#endif /*BBCORE*/
 
double getData ( _p, _ppvar, _thread, _nt ) double* _p; Datum* _ppvar; ThreadDatum* _thread; _NrnThread* _nt; {
   double _lgetData;
 
/*VERBATIM*/
{ 
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
 
return _lgetData;
 }
 
#if 0 /*BBCORE*/
 
static double _hoc_getData(_vptr) void* _vptr; {
 double _r;
   double* _p; Datum* _ppvar; ThreadDatum* _thread; _NrnThread* _nt;
   _p = ((Point_process*)_vptr)->_prop->param;
  _ppvar = ((Point_process*)_vptr)->_prop->dparam;
  _thread = _extcall_thread;
  _nt = (_NrnThread*)((Point_process*)_vptr)->_vnt;
 _r =  getData ( _p, _ppvar, _thread, _nt ) ;
 return(_r);
}
 
#endif /*BBCORE*/
 
double getColumnDataRange ( _p, _ppvar, _thread, _nt ) double* _p; Datum* _ppvar; ThreadDatum* _thread; _NrnThread* _nt; {
   double _lgetColumnDataRange;
 
/*VERBATIM*/
{ 
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
 
return _lgetColumnDataRange;
 }
 
#if 0 /*BBCORE*/
 
static double _hoc_getColumnDataRange(_vptr) void* _vptr; {
 double _r;
   double* _p; Datum* _ppvar; ThreadDatum* _thread; _NrnThread* _nt;
   _p = ((Point_process*)_vptr)->_prop->param;
  _ppvar = ((Point_process*)_vptr)->_prop->dparam;
  _thread = _extcall_thread;
  _nt = (_NrnThread*)((Point_process*)_vptr)->_vnt;
 _r =  getColumnDataRange ( _p, _ppvar, _thread, _nt ) ;
 return(_r);
}
 
#endif /*BBCORE*/
 
double getColumnData ( _p, _ppvar, _thread, _nt ) double* _p; Datum* _ppvar; ThreadDatum* _thread; _NrnThread* _nt; {
   double _lgetColumnData;
 
/*VERBATIM*/
{ 
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
 
return _lgetColumnData;
 }
 
#if 0 /*BBCORE*/
 
static double _hoc_getColumnData(_vptr) void* _vptr; {
 double _r;
   double* _p; Datum* _ppvar; ThreadDatum* _thread; _NrnThread* _nt;
   _p = ((Point_process*)_vptr)->_prop->param;
  _ppvar = ((Point_process*)_vptr)->_prop->dparam;
  _thread = _extcall_thread;
  _nt = (_NrnThread*)((Point_process*)_vptr)->_vnt;
 _r =  getColumnData ( _p, _ppvar, _thread, _nt ) ;
 return(_r);
}
 
#endif /*BBCORE*/
 
double getAttributeValue ( _p, _ppvar, _thread, _nt ) double* _p; Datum* _ppvar; ThreadDatum* _thread; _NrnThread* _nt; {
   double _lgetAttributeValue;
 
/*VERBATIM*/
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
 
return _lgetAttributeValue;
 }
 
#if 0 /*BBCORE*/
 
static double _hoc_getAttributeValue(_vptr) void* _vptr; {
 double _r;
   double* _p; Datum* _ppvar; ThreadDatum* _thread; _NrnThread* _nt;
   _p = ((Point_process*)_vptr)->_prop->param;
  _ppvar = ((Point_process*)_vptr)->_prop->dparam;
  _thread = _extcall_thread;
  _nt = (_NrnThread*)((Point_process*)_vptr)->_vnt;
 _r =  getAttributeValue ( _p, _ppvar, _thread, _nt ) ;
 return(_r);
}
 
#endif /*BBCORE*/
 
double closeFile ( _p, _ppvar, _thread, _nt ) double* _p; Datum* _ppvar; ThreadDatum* _thread; _NrnThread* _nt; {
   double _lcloseFile;
 
/*VERBATIM*/
{ 
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
 
return _lcloseFile;
 }
 
#if 0 /*BBCORE*/
 
static double _hoc_closeFile(_vptr) void* _vptr; {
 double _r;
   double* _p; Datum* _ppvar; ThreadDatum* _thread; _NrnThread* _nt;
   _p = ((Point_process*)_vptr)->_prop->param;
  _ppvar = ((Point_process*)_vptr)->_prop->dparam;
  _thread = _extcall_thread;
  _nt = (_NrnThread*)((Point_process*)_vptr)->_vnt;
 _r =  closeFile ( _p, _ppvar, _thread, _nt ) ;
 return(_r);
}
 
#endif /*BBCORE*/
 
double exchangeSynapseLocations ( _p, _ppvar, _thread, _nt ) double* _p; Datum* _ppvar; ThreadDatum* _thread; _NrnThread* _nt; {
   double _lexchangeSynapseLocations;
 
/*VERBATIM*/
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
    
 
return _lexchangeSynapseLocations;
 }
 
#if 0 /*BBCORE*/
 
static double _hoc_exchangeSynapseLocations(_vptr) void* _vptr; {
 double _r;
   double* _p; Datum* _ppvar; ThreadDatum* _thread; _NrnThread* _nt;
   _p = ((Point_process*)_vptr)->_prop->param;
  _ppvar = ((Point_process*)_vptr)->_prop->dparam;
  _thread = _extcall_thread;
  _nt = (_NrnThread*)((Point_process*)_vptr)->_vnt;
 _r =  exchangeSynapseLocations ( _p, _ppvar, _thread, _nt ) ;
 return(_r);
}
 
#endif /*BBCORE*/

static void initmodel(double* _p, Datum* _ppvar, ThreadDatum* _thread, _NrnThread* _nt) {
  int _i; double _save;{
 {
   closeFile ( _threadargs_ ) ;
   }

}
}

static void nrn_init(_NrnThread* _nt, _Memb_list* _ml, int _type){
double* _p; Datum* _ppvar; ThreadDatum* _thread;
double _v; int* _ni; int _iml, _cntml;
#if CACHEVEC
    _ni = _ml->_nodeindices;
#endif
_cntml = _ml->_nodecount;
_thread = _ml->_thread;
for (_iml = 0; _iml < _cntml; ++_iml) {
 _p = _ml->_data + _iml*_psize; _ppvar = _ml->_pdata + _iml*_ppsize;
 _tsav = -1e20;
 initmodel(_p, _ppvar, _thread, _nt);
}}

static double _nrn_current(double* _p, Datum* _ppvar, ThreadDatum* _thread, _NrnThread* _nt, double _v){double _current=0.;v=_v;{
} return _current;
}

static void nrn_state(_NrnThread* _nt, _Memb_list* _ml, int _type) {
 double _break, _save;
double* _p; Datum* _ppvar; ThreadDatum* _thread;
double _v; int* _ni; int _iml, _cntml;
#if CACHEVEC
    _ni = _ml->_nodeindices;
#endif
_cntml = _ml->_nodecount;
_thread = _ml->_thread;
for (_iml = 0; _iml < _cntml; ++_iml) {
 _p = _ml->_data + _iml*_psize; _ppvar = _ml->_pdata + _iml*_ppsize;
 _break = t + .5*dt; _save = t;
 v=_v;
{
}}

}

static terminal(){}

static _initlists(){
 double _x; double* _p = &_x;
 int _i; static int _first = 1;
  if (!_first) return;
_first = 0;
}
