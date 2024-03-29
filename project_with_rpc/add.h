/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#ifndef _ADD_H_RPCGEN
#define _ADD_H_RPCGEN

#include <rpc/rpc.h>


#ifdef __cplusplus
extern "C" {
#endif


struct duration {
	int seconds;
};
typedef struct duration duration;

#define SLEEP_PROG 0x31599123
#define SLEEP_VERS 1

#if defined(__STDC__) || defined(__cplusplus)
#define add 1
extern  void * add_1(duration *, CLIENT *);
extern  void * add_1_svc(duration *, struct svc_req *);
extern int sleep_prog_1_freeresult (SVCXPRT *, xdrproc_t, caddr_t);

#else /* K&R C */
#define add 1
extern  void * add_1();
extern  void * add_1_svc();
extern int sleep_prog_1_freeresult ();
#endif /* K&R C */

/* the xdr functions */

#if defined(__STDC__) || defined(__cplusplus)
extern  bool_t xdr_duration (XDR *, duration*);

#else /* K&R C */
extern bool_t xdr_duration ();

#endif /* K&R C */

#ifdef __cplusplus
}
#endif

#endif /* !_ADD_H_RPCGEN */
