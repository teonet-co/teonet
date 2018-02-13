%module teonet

%{
#undef TRUE
#undef FALSE    
#include "../../src/ev_mgr.h"

//extern const char *teoGetLibteonetVersion();
//extern ksnetEvMgrClass *ke = ksnetEvMgrInit(argc, argv, event_cb /*NULL*/, READ_ALL);

%}

// This tells SWIG to treat char ** as a special case
%typemap(in) char ** {
    AV *tempav;
    I32 len;
    int i;
    SV  **tv;
    if (!SvROK($input))
        croak("Argument $argnum is not a reference.");
        if (SvTYPE(SvRV($input)) != SVt_PVAV)
        croak("Argument $argnum is not an array.");
        tempav = (AV*)SvRV($input);
    len = av_len(tempav);
    $1 = (char **) malloc((len+2)*sizeof(char *));
    for (i = 0; i <= len; i++) {
        tv = av_fetch(tempav, i, 0);
        $1[i] = (char *) SvPV(*tv,PL_na);
        }
    $1[i] = NULL;
};

// This cleans up the char ** array after the function call
%typemap(freearg) char ** {
    free($1);
}

// Now a few inline functions
%inline %{

// Perl callback subroutine
static SV * callback = (SV*)NULL;

// Event callback C wrapper
static void event_cb(ksnetEvMgrClass *ke, ksnetEvMgrEvents event, void *data,
    size_t data_len, void *user_data) {

    // C event loop switch (just for example)
    switch(event) {

        // Calls immediately after event manager starts
        case EV_K_STARTED:
            printf("EV_K_STARTED - C swig inline\n");
            break;
    }

    // Call Perl callback subroutine
    if(callback) {
        dSP;
        PUSHMARK(SP);
        EXTEND(SP, 6);
        PUSHs(sv_2mortal(sv_setref_pv( newSV(0), "cpKePtr", ke ) ) );
        PUSHs(sv_2mortal(newSViv(event)));
        PUSHs(sv_2mortal(sv_setref_pv( newSV(0), "cpDataPtr", data ) ) );
        PUSHs(sv_2mortal(newSViv(data_len)));
        PUSHs(sv_2mortal(sv_setref_pv( newSV(0), "cpUserDataPtr", ke ) ) );
        PUSHs(sv_2mortal(newSVpv(data?((ksnCorePacketData*)data)->from: "", 0)));
        PUTBACK;

        // Call the Perl sub to process the callback
        call_sv(callback, G_DISCARD);
    }
}

// Teonet event manager init function wrapper
ksnetEvMgrClass *ksnetEvMgrInitP(int argc, char** argv,
        SV* pFcn) {
    // Remember the Perl sub
    if (callback == (SV*)NULL) callback = newSVsv(pFcn);
    else SvSetSV(callback, pFcn);

    ksnetEvMgrClass *ke = ksnetEvMgrInit(argc, argv, event_cb, READ_ALL);
    return ke;
}

// Get `from peer` from data
const char* rdFrom(void *data) {
    ksnCorePacketData *rd = data;
    return rd ? rd->from : "";
}

%}

// Teonet functions
const char *teoGetLibteonetVersion();
void teoSetAppType(ksnetEvMgrClass *ke, char *type);
void teoSetAppVersion(ksnetEvMgrClass *ke, char *version);
int ksnetEvMgrRun(ksnetEvMgrClass *ke);
void ksnetEvMgrStop(ksnetEvMgrClass *ke);
