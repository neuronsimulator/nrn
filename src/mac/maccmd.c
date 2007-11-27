#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int opened;
static char fin[1024];
short SIOUXHandleOneEvent(EventRecord *);

static void mac_open_doc(const char*);
static void mac_open_app();
static void install_handlers();
static char* fullname(const FSSpec* file);

mac_cmdline(int* pargc, char*** pargv) {
	static char* argv[2];
	opened = 0;
	install_handlers();
	while(!opened) {
		SIOUXHandleOneEvent(nil);
	}
	*pargc = 2;
	*pargv = argv;
	argv[0] = "modlunit";
	argv[1] = fin;
}

static void mac_open_doc(const char* s){
	strncpy(fin, s, 1024);
	opened = 1;
}

static void mac_open_app() {
#if TARGET_API_MAC_CARBON
	char* s;
	OSStatus err;
	NavReplyRecord reply;
	err = NavChooseFile(NULL, &reply, NULL, NULL, NULL, NULL, NULL, NULL);
	if (err == noErr && reply.validRecord) {
		AEKeyword key;
		DescType type;
		Size size;
		FSSpec fs;
		err = AEGetNthPtr(&(reply.selection), 1, typeFSS, &key, &type, &fs, sizeof(fs), &size);
		if (err == noErr) {
			s = fullname(&fs);
			fs.name[fs.name[0]+1]='\0';
			strcat(s,(char*)&fs.name[1]);						
/*printf("fullname: %s\n", s);*/
			strncpy(fin, s, 1024);
			opened = 1;
		}
	}
	if (err != noErr) {
		diag("Usage: Drag a .mod file onto the application icon.\n",
		 "Or Open a file with a .mod suffix.\nDo a File/Quit and try again");
	}
#else
	char* s;
	struct StandardFileReply reply;
	StandardGetFile(0, -1, 0, &reply);
	if (reply.sfGood && !reply.sfIsFolder && !reply.sfIsVolume) {
		s = fullname(&reply.sfFile);
		reply.sfFile.name[reply.sfFile.name[0]+1]='\0';
		strcat(s,(char*)&reply.sfFile.name[1]);						
/*printf("fullname: %s\n", s);*/
		strncpy(fin, s, 1024);
		opened = 1;
	}else{
		diag("Usage: Drag a .mod file onto the application icon.\n",
		 "Or Open a file with a .mod suffix.\nDo a File/Quit and try again");
	}
#endif
}

// all the following copied from session.c


/****************************************************************/
/* Purpose..: Create a full path name for a FSSpec file			*/
/* Input....: FSSpec											*/
/* Returns..: String[255] Ptr									*/
/* Jijun 5/30/97 												*/
/****************************************************************/
static char* fullname(const FSSpec *file){
	short	refNum;
    CInfoPBRec myPB;
    char prePath[256];
    static char fullPath[512];
    OSErr	err;	
    Str255 dirName;
    char * s;
       
    fullPath[0]='\0';
    myPB.dirInfo.ioVRefNum = file->vRefNum;
    myPB.dirInfo.ioDrParID = file->parID;
    myPB.dirInfo.ioFDirIndex= -1;
    myPB.dirInfo.ioNamePtr = dirName;
    do {
    		myPB.dirInfo.ioDrDirID = myPB.dirInfo.ioDrParID;
    		err = PBGetCatInfoSync(&myPB);
			//debugfile("\nerror after PBGetCatInfoSync: %ld", err);
    		if (err) {
    			return nil;
    		}
    		else {
    			dirName[dirName[0]+1]='\0';
    			strcpy(prePath, (char*)&dirName[1]);
    			strcat(prePath, ":");
    			strcat(prePath, fullPath);
    			strcpy(fullPath, prePath);
				//debugfile("\nfullPath= %s", fullPath);
				//debugfile("\nmyPB.dirInfo.ioDrDirID= %ld", myPB.dirInfo.ioDrDirID);
    		}
    } while (myPB.dirInfo.ioDrDirID > fsRtDirID);
   
	s=fullPath;
   	return s;
}

#if TARGET_API_MAC_CARBON
/* cw6 required unsigned long */
static pascal OSErr MyHandleQuit (const AppleEvent *theAppleEvent, AppleEvent *reply, long handlerRefcon) {
	QuitApplicationEventLoop();
	RemoveConsole();
	return noErr;
}
#endif

/****************************************************************/
/* Purpose..: Handling Open Application AppleEvent				*/
/* Input....: theAppleEvent, reply, handleRefcon				*/
/* Returns..: OSErr												*/
/* Jijun 5/29/97 reference: Interapplication (4-15)				*/
/****************************************************************/
#if TARGET_API_MAC_CARBON
/* cw6 required unsigned long */
static pascal OSErr MyHandleOApp (const AppleEvent *theAppleEvent, AppleEvent *reply, long handlerRefcon) {
#else
static pascal OSErr MyHandleOApp (const AppleEvent *theAppleEvent, const AppleEvent *reply, long handlerRefcon) {
#endif
	AEDescList	docList;
	
	//debugfile("\nWe are in MyHandleOApp");
	//debugfile("\ntheAppleEvent Descriptor type: %ld", theAppleEvent.descriptorType);
	//debugfile("\ntheAppleEvent Descriptor handle*: %ld", *(theAppleEvent.dataHandle));
	//debugfile("\ntheAppleEvent Descriptor handle**: %ld", **(theAppleEvent.dataHandle));
	//debugfile("\nreply Descriptor type: %ld", reply.descriptorType);
	//debugfile("\nhandlerRefcon: %ld", handlerRefcon);

	mac_open_app();	
	//OSErr myErr=AEGetParamDesc(&theAppleEvent, keyDirectObject, typeAEList, &docList);
	//debugfile("\nerror after AEGetParamDesc: %ld", myErr);
	return (0);
}

/****************************************************************/
/* Purpose..: Handling Open Documents AppleEvent				*/
/* Input....: theAppleEvent, reply, handleRefcon				*/
/* Returns..: OSErr												*/
/* Jijun 5/28/97 reference: Interapplication (4-15)				*/
/****************************************************************/
#if TARGET_API_MAC_CARBON
static pascal OSErr MyHandleODoc (const AppleEvent *theAppleEvent, AppleEvent *reply, long handlerRefcon) {
#else
static pascal OSErr MyHandleODoc (const AppleEvent *theAppleEvent, const AppleEvent *reply, long handlerRefcon) {
#endif
//void MyHandleODoc (AppleEvent &theAppleEvent, AppleEvent &reply, long handlerRefcon) {
	FSSpec		myFSS;
	AEDescList	docList;
	long		itemInList;
	AEKeyword	keywd;
	DescType	returnedType;
	Size		actualSize;
	OSErr ingoreErr;
	//debugfile("\n\nWe are in MyHandleODoc");
	//debugfile("\ntheAppleEvent Descriptor type: %ld", theAppleEvent.descriptorType);
	//debugfile("\ntheAppleEvent Descriptor handle*: %ld", *(theAppleEvent.dataHandle));
	//debugfile("\ntheAppleEvent Descriptor handle**: %ld", **(theAppleEvent.dataHandle));
	//debugfile("\nreply Descriptor type: %ld", reply.descriptorType);
	//debugfile("\nhandlerRefcon: %ld", handlerRefcon);
	
	// get the direct parameter--a descriptor list-- and put it into docList
	OSErr myErr=AEGetParamDesc(theAppleEvent, keyDirectObject, typeAEList, &docList);
	//debugfile("\nerror after AEGetParamDesc: %ld", myErr);
	if (myErr) {
		return myErr;
	}
	else {
		// check for missing required parameters
		//myErr = MyGotRequiredParams(theAppleEvent);
		if (myErr) {
			return myErr;
		}
		else {
			// count the number of descriptor records int the list
			myErr = AECountItems(&docList, &itemInList);
			//debugfile("\nerror after AECountItems: %ld", myErr);
			//debugfile("\nitemInList: %ld", itemInList);
			if (myErr) {
				return myErr;
			}
			else {
				// get each descriptor record from list, coerce the returned data to an FSSpec record,
				// and open the associated file
				long index;
				for (index=1; index<=itemInList; index++) {
					myErr = AEGetNthPtr(&docList, index, typeFSS, &keywd, &returnedType,
										&myFSS, sizeof(myFSS), &actualSize);
					//debugfile("\nerror after AEGetNthPtr: %ld", myErr);
					if (myErr) {
						return myErr;
					}
					else {
						char* s=fullname(&myFSS);
						myFSS.name[myFSS.name[0]+1]='\0';
						strcat(s,(char*)&myFSS.name[1]);						
						//debugfile("\nMyOpenFile is %s", s);
						mac_open_doc(s);
						//hoc_xopen1(s, 0);
						//myErr=FSpDelete(&myFSS);
					}	// noerr for AEGetNthPtr
				}	// for loop
			}	// noerr for AECountItems
		}	// noerr for MyGotRequiredParams
	ingoreErr=AEDisposeDesc(&docList);
	//debugfile("\nerror after AEDisposeDesc: %ld", ingoreErr);
	}	// noerr for AEGetParamDesc
	
	return myErr;
}

static void install_handlers() {
	AEEventHandlerUPP	myHandleODoc;	// add by jijun 5/28/97 for handle open doc
	AEEventHandlerUPP	myHandleOApp;	// add by jijun 5/29/97 for handle open app
	AEEventHandlerUPP	handler;
	OSErr error;
	long				handleRefcon;
#if TARGET_API_MAC_CARBON
	InitCursor();
#endif
	// add by jijun 5/28/97 for handle open doc
	//debugfile("\nWe will begin install AppleEvent handler");
	AEGetEventHandler(kCoreEventClass, kAEOpenDocuments, &handler, &handleRefcon, FALSE);
	//debugfile("\nDoc handleRefcon:%ld", handleRefcon);
	AEGetEventHandler(kCoreEventClass, kAEOpenApplication, &handler, &handleRefcon, FALSE);
	//debugfile("\nApp handleRefcon:%ld", handleRefcon);
#if TARGET_API_MAC_CARBON
	AEInstallEventHandler(kCoreEventClass, kAEQuitApplication,
		NewAEEventHandlerUPP(&MyHandleQuit), 0, FALSE);
	myHandleOApp=NewAEEventHandlerUPP(&MyHandleOApp);
	myHandleODoc=NewAEEventHandlerUPP(&MyHandleODoc);
#else
	myHandleOApp=NewAEEventHandlerProc(MyHandleOApp);
	myHandleODoc=NewAEEventHandlerProc(MyHandleODoc);
#endif
	error=AEInstallEventHandler(kCoreEventClass, kAEOpenApplication, myHandleOApp, 0, FALSE);
	//debugfile("\nerror=%ld", error);
	if (!error) {
		//debugfile("\nNo error in install open application handler");
		error=AEInstallEventHandler(kCoreEventClass, kAEOpenDocuments, myHandleODoc, 0, FALSE);
		//debugfile("\nerror=%ld", error);
	}
}
