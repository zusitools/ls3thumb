

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 8.01.0622 */
/* at Tue Jan 19 04:14:07 2038
 */
/* Compiler settings for Ls3Thumb.idl:
    Oicf, W1, Zp8, env=Win64 (32b run), target_arch=AMD64 8.01.0622 
    protocol : all , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */



/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 500
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif /* __RPCNDR_H_VERSION__ */

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __Ls3Thumb_i_h__
#define __Ls3Thumb_i_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ILs3ThumbShlExt_FWD_DEFINED__
#define __ILs3ThumbShlExt_FWD_DEFINED__
typedef interface ILs3ThumbShlExt ILs3ThumbShlExt;

#endif 	/* __ILs3ThumbShlExt_FWD_DEFINED__ */


#ifndef __Ls3ThumbShlExt_FWD_DEFINED__
#define __Ls3ThumbShlExt_FWD_DEFINED__

#ifdef __cplusplus
typedef class Ls3ThumbShlExt Ls3ThumbShlExt;
#else
typedef struct Ls3ThumbShlExt Ls3ThumbShlExt;
#endif /* __cplusplus */

#endif 	/* __Ls3ThumbShlExt_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 


#ifndef __ILs3ThumbShlExt_INTERFACE_DEFINED__
#define __ILs3ThumbShlExt_INTERFACE_DEFINED__

/* interface ILs3ThumbShlExt */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ILs3ThumbShlExt;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9BAAAAB3-80BD-4CBA-BCE7-BFEA4BCC2628")
    ILs3ThumbShlExt : public IUnknown
    {
    public:
    };
    
    
#else 	/* C style interface */

    typedef struct ILs3ThumbShlExtVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ILs3ThumbShlExt * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ILs3ThumbShlExt * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ILs3ThumbShlExt * This);
        
        END_INTERFACE
    } ILs3ThumbShlExtVtbl;

    interface ILs3ThumbShlExt
    {
        CONST_VTBL struct ILs3ThumbShlExtVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ILs3ThumbShlExt_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ILs3ThumbShlExt_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ILs3ThumbShlExt_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ILs3ThumbShlExt_INTERFACE_DEFINED__ */



#ifndef __Ls3ThumbLib_LIBRARY_DEFINED__
#define __Ls3ThumbLib_LIBRARY_DEFINED__

/* library Ls3ThumbLib */
/* [version][uuid] */ 


EXTERN_C const IID LIBID_Ls3ThumbLib;

EXTERN_C const CLSID CLSID_Ls3ThumbShlExt;

#ifdef __cplusplus

class DECLSPEC_UUID("0859D246-72BB-4C89-9345-66374CE7C6D5")
Ls3ThumbShlExt;
#endif
#endif /* __Ls3ThumbLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


