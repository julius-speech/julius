/**
 * 
 * cJulius: JuliusLib wrapper class for C++
 *
 * Copyright (c) 2011-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 *
 * This is a part of the Julius software. 
 */

#include	<julius/juliuslib.h>
#include	"Julius.h"
#include	"process.h"

// -----------------------------------------------------------------------------
// JuliusLib callback functions
//

#define JCALLBACK(A, B) static void A (Recog *recog, void *data) { cJulius *j = (cJulius *) data; PostMessage(j->getWindow(), WM_JULIUS, B, 0L);}

JCALLBACK(callback_engine_active,	JEVENT_ENGINE_ACTIVE)
JCALLBACK(callback_engine_inactive,	JEVENT_ENGINE_INACTIVE)
JCALLBACK(callback_audio_ready,		JEVENT_AUDIO_READY)
JCALLBACK(callback_audio_begin,		JEVENT_AUDIO_BEGIN)
JCALLBACK(callback_audio_end,		JEVENT_AUDIO_END)
JCALLBACK(callback_recog_begin,		JEVENT_RECOG_BEGIN)
JCALLBACK(callback_recog_end,		JEVENT_RECOG_END)
JCALLBACK(callback_recog_frame,		JEVENT_RECOG_FRAME)
JCALLBACK(callback_engine_pause,	JEVENT_ENGINE_PAUSE)
JCALLBACK(callback_engine_resume,	JEVENT_ENGINE_RESUME)

// callback to get result
static void callback_result_final(Recog *recog, void *data)
{
	cJulius *j = (cJulius *)data;

	int i;
	WORD_INFO *winfo;
	WORD_ID *seq;
	int seqnum;
	Sentence *s;
	RecogProcess *r;
	static char str[2048];
	static wchar_t wstr[2048];
	size_t size = 0;
	WPARAM wparam = 0;
	_locale_t locale;
	unsigned int code;

	r = j->getRecog()->process_list;
	if (! r->live) return;

	if (r->result.status < 0) {      /* no results obtained */

		switch(r->result.status) {
		case J_RESULT_STATUS_REJECT_POWER:
			strcpy(str, "<input rejected by power>");
			break;
		case J_RESULT_STATUS_TERMINATE:
			strcpy(str, "<input teminated by request>");
			break;
		case J_RESULT_STATUS_ONLY_SILENCE:
			strcpy(str, "<input rejected by decoder (silence input result)>");
			break;
		case J_RESULT_STATUS_REJECT_GMM:
			strcpy(str, "<input rejected by GMM>");
			break;
		case J_RESULT_STATUS_REJECT_SHORT:
			strcpy(str, "<input rejected by short input>");
			break;
		case J_RESULT_STATUS_FAIL:
			strcpy(str, "<search failed>");
			break;
		}
		code = (- r->result.status);

	} else {

	    winfo = r->lm->winfo;
		s = &(r->result.sent[0]);
		seq = s->word;
		seqnum = s->word_num;
		str[0] = '\0';
		for(i=0;i<seqnum;i++) strcat(str, winfo->woutput[seq[i]]);
		code = 0;
	}

	// convert to wide char
	//mbstowcs_s( &size, wstr, str, strlen(str)+1);
	locale = j->getModelLocale();
	if (locale) _mbstowcs_s_l( &size, wstr, str, strlen(str)+1, locale);
	else mbstowcs_s( &size, wstr, str, strlen(str)+1);


	// set status parameter
	wparam = (code << 16) + JEVENT_RESULT_FINAL;

	// send message
	PostMessage(j->getWindow(), WM_JULIUS, wparam, (LPARAM)wstr);
}

// callbackk for pause
static void callback_wait_for_resume(Recog *recog, void *data)
{
	cJulius *j = (cJulius *)data;
	// Stop running the engine thread
	SuspendThread( j->getThreadHandle() );
	// the engine thread will wait until the main thread calls ResumeThread() for it
}

#ifdef APP_ADIN
static int callback_adin_fetch_input(SP16 *sampleBuffer, int reqlen)
{
	// If shared audio buffer has some new data, or some data remains from the last call,
	// get the samples into sampleBuffer at most reqlen length.
}
#endif

// main function for the engine thread
unsigned __stdcall recogThreadMain( void *param )
{
	int ret;
	Recog *recog = (Recog *)param;
	ret = j_recognize_stream(recog);
	_endthreadex(ret);
	return(ret);
}

//-----------------------------------------------------------------------------------------
// Julius class definition
//-----------------------------------------------------------------------------------------

//=============
// Constructor
//=============
cJulius::cJulius( void ) : m_jconf( NULL ), m_recog( NULL ), m_opened( false ), m_threadHandle( NULL ), m_fpLogFile( NULL ), m_modelLocale( NULL )
{
#ifdef APP_ADIN
	m_appsource = 0;
#endif
	setLogFile( "juliuslog.txt" );
}

//============
// Destructor
//============
cJulius::~cJulius( void )
{
	release();
	if ( m_fpLogFile ) {
		fclose(m_fpLogFile);
		m_fpLogFile = NULL;
	}
}

//=================================
// Initialize, with argument array
//=================================
bool cJulius::initialize( int argnum, char *argarray[])
{
	bool ret;

	release();

	m_jconf = j_config_load_args_new( argnum, argarray );

	if (m_jconf == NULL) {		/* error */
	    return false;
	}

	ret = createEngine();

	return ret;
}

//===============================
// Initialize, with a Jconf file
//===============================
bool cJulius::initialize( char *filename )
{
	bool ret;

	release();

	if (! loadJconf( filename )) return false;

	ret = createEngine();

	return ret;
}

//===================
// Load a Jconf file
//===================
bool cJulius::loadJconf( char *filename )
{
	if (m_jconf) {
		if (j_config_load_file(m_jconf, filename) == -1) {
			return false;
		}
	} else {
		m_jconf = j_config_load_file_new( filename );
		if (m_jconf == NULL) {		/* error */
		    return false;
		}
	}
	return true;
}

//==============================================
// Set a log file to save Julius engine outputs
//==============================================
void cJulius::setLogFile( const char *filename )
{
	if (m_fpLogFile) {
		fclose( m_fpLogFile );
	}
	m_fpLogFile = fopen( filename, "w" );
	if (m_fpLogFile) {
		jlog_set_output( m_fpLogFile );
	}
}

//======================
// set locale of the LM
//======================
void cJulius::setModelLocale( const char *locale )
{
	m_modelLocale = _create_locale( LC_CTYPE, locale);
}

//========================================
// return current model locale for output
//========================================
_locale_t cJulius::getModelLocale( void )
{
	return( m_modelLocale );
}

//========================
// Create engine instance
//========================
bool cJulius::createEngine( void )
{
#ifdef APP_ADIN
	ADIn *a;
#endif

	if (!m_jconf) return false;
	if (m_recog) return false;

#ifdef APP_ADIN
	if (m_appsource != 0) {
		switch(m_appsource) {
			case 1: // buffer input, batch
				m_recog->jconf->input.type = INPUT_WAVEFORM;
				m_recog->jconf->input.speech_input = SP_RAWFILE;
				m_recog->jconf->decodeopt.realtime_flag = FALSE;
				break;
			case 2: // buffer input, incremental
				m_recog->jconf->input.type = INPUT_WAVEFORM;
				m_recog->jconf->input.speech_input = SP_RAWFILE;
				m_recog->jconf->decodeopt.realtime_flag = TRUE;
				break;
		}
	}
#endif

	// Create engine instance
	m_recog = j_create_instance_from_jconf(m_jconf);
	if (m_recog == NULL) {
		return false;
	}

	// Register callbacks
	callback_add(m_recog, CALLBACK_EVENT_PROCESS_ONLINE,		::callback_engine_active, this);
	callback_add(m_recog, CALLBACK_EVENT_PROCESS_OFFLINE,		::callback_engine_inactive, this);
	callback_add(m_recog, CALLBACK_EVENT_SPEECH_READY,			::callback_audio_ready, this);
	callback_add(m_recog, CALLBACK_EVENT_SPEECH_START,			::callback_audio_begin, this);
	callback_add(m_recog, CALLBACK_EVENT_SPEECH_STOP,			::callback_audio_end, this);
	callback_add(m_recog, CALLBACK_EVENT_RECOGNITION_BEGIN,		::callback_recog_begin, this);
	callback_add(m_recog, CALLBACK_EVENT_RECOGNITION_END,		::callback_recog_end, this);
	callback_add(m_recog, CALLBACK_EVENT_PASS1_FRAME,			::callback_recog_frame, this);
	callback_add(m_recog, CALLBACK_EVENT_PAUSE,					::callback_engine_pause, this);
	callback_add(m_recog, CALLBACK_EVENT_RESUME,				::callback_engine_resume, this);
	callback_add(m_recog, CALLBACK_RESULT,						::callback_result_final, this);
	callback_add(m_recog, CALLBACK_PAUSE_FUNCTION,				::callback_wait_for_resume, this);

#ifdef APP_ADIN
	// Initialize application side audio input
	if (m_appsource != 0) {
		a = m_recog->adin;
		switch(m_appsource) {
			case 1: // buffer input, batch
				a->ad_standby			= NULL;
				a->ad_begin				= NULL;
				a->ad_end				= NULL;
				a->ad_resume			= NULL;
				a->ad_pause				= NULL;
				a->ad_terminate			= NULL;
				a->ad_read				= callback_adin_fetch_input;
				a->ad_input_name		= NULL;
				a->silence_cut_default	= FALSE;
				a->enable_thread		= FALSE;
				break;
			case 2: // buffer input, incremental
				a->ad_standby			= NULL;
				a->ad_begin				= NULL;
				a->ad_end				= NULL;
				a->ad_resume			= NULL;
				a->ad_pause				= NULL;
				a->ad_terminate			= NULL;
				a->ad_read				= callback_adin_fetch_input;
				a->ad_input_name		= NULL;
				a->silence_cut_default	= FALSE;
				a->enable_thread		= FALSE;
				break;
		}
	    a->ds = NULL;
		a->down_sample = FALSE;
		if (adin_standby(a, m_recog->jconf->input.sfreq, NULL) == FALSE) return false;
		if (adin_setup_param(a, m_recog->jconf) == FALSE) return false;
		a->input_side_segment = FALSE;
	} else {
		// Let JuliusLib get audio input
		if (! j_adin_init( m_recog ) ) return false;
	}
#else
	if (! j_adin_init( m_recog ) ) return false;
#endif

	return true;
}

//==============================================
// Open stream and start the recognition thread
//==============================================
bool cJulius::startProcess( HWND hWnd )
{

	if ( ! m_recog ) return false;

	// store window hanlder to send event message
	m_hWnd = hWnd;

	if (m_opened == false) {
		// open device
		switch(j_open_stream(m_recog, NULL)) {
		case 0:			/* succeeded */
			break;
		case -1:      		/* error */
			// fprintf(stderr, "error in input stream\n");
			return false;
		case -2:			/* end of recognition process */
			//fprintf(stderr, "failed to begin input stream\n");
			return false;
		}
		// create recognition thread
		m_threadHandle = (HANDLE)_beginthreadex(NULL, 0, ::recogThreadMain, m_recog, 0, NULL);
		if (m_threadHandle == 0) {
			j_close_stream(m_recog);
			return false;
		}
		SetThreadPriority(m_threadHandle, THREAD_PRIORITY_HIGHEST);

		m_opened = true;
	}

	return true;
}

//==================================================
// Close audio stream and detach recognition thread
//==================================================
void cJulius::stopProcess( void )
{

	if ( ! m_recog ) return;

	if (m_opened) {
		// recognition thread will exit when audio input is closed
		j_close_stream(m_recog);
		m_opened = false;
	}
}

//===================
// Pause recognition
//===================
void cJulius::pause( void )
{
	if ( ! m_recog ) return;
	// request library to pause
	// After pause, the recognition thread will issue pause event
	// and then enter callback_wait_for_resume(), where thread will pause.
	j_request_terminate(m_recog);
}

//====================
// Resume recognition
//====================
void cJulius::resume( void )
{
	if ( ! m_recog ) return;
	// request library to resume
	j_request_resume(m_recog);
	// resume the recognition thread
	ResumeThread( m_threadHandle );
}

//==================
// Load DFA grammar
//==================
bool cJulius::loadGrammar( WORD_INFO *winfo, DFA_INFO *dfa, char *dictfile, char *dfafile, RecogProcess *r )
{
	boolean ret;

	if ( ! m_recog ) return false;

	// load grammar
	switch( r->lmvar ) {
	case LM_DFA_WORD:
		ret = init_wordlist(winfo, dictfile, r->lm->am->hmminfo, 
			r->lm->config->wordrecog_head_silence_model_name,
			r->lm->config->wordrecog_tail_silence_model_name,
			(r->lm->config->wordrecog_silence_context_name[0] == '\0') ? NULL : r->lm->config->wordrecog_silence_context_name,
			r->lm->config->forcedict_flag);
		if (ret == FALSE) {
			return false;
		}
		break;
	case LM_DFA_GRAMMAR:
		ret = init_voca(winfo, dictfile, r->lm->am->hmminfo, FALSE, r->lm->config->forcedict_flag);
		if (ret == FALSE) {
			return false;
		}
	    ret = init_dfa(dfa, dfafile);
		if (ret == FALSE) {
			return false;
		}
		break;
	}

	return true;
}

//=================
// Add DFA grammar
//=================
bool cJulius::addGrammar( char *name, char *dictfile, char *dfafile, bool deleteAll )
{
	WORD_INFO *winfo;
	DFA_INFO *dfa;
	RecogProcess *r;
	boolean ret;

	if ( ! m_recog ) return false;

	r = m_recog->process_list;

	// load grammar
	switch( r->lmvar ) {
	case LM_DFA_WORD:
		winfo = word_info_new();
		dfa = NULL;
		ret = loadGrammar( winfo, NULL, dictfile, NULL, r );
		if ( ! ret ) {
			word_info_free(winfo);
			return false;
		}
		break;
	case LM_DFA_GRAMMAR:
		winfo = word_info_new();
		dfa = dfa_info_new();
		ret = loadGrammar( winfo, dfa, dictfile, dfafile, r );
		if ( ! ret ) {
			word_info_free(winfo);
			dfa_info_free(dfa);
			return false;
		}
	}
	if ( deleteAll ) {
		/* delete all existing grammars */
		multigram_delete_all(r->lm);
	}
	/* register the new grammar to multi-gram tree */
	multigram_add(dfa, winfo, name, r->lm);
	/* need to rebuild the global lexicon */
	/* tell engine to update at requested timing */
	schedule_grammar_update(m_recog);
	/* make sure this process will be activated */
	r->active = 1;

	PostMessage(getWindow(), WM_JULIUS, JEVENT_GRAM_UPDATE, 0L);

	return true;
}

//==============
// Swap grammar
//==============
bool cJulius::changeGrammar( char *name, char *dictfile, char *dfafile )
{
	if ( ! m_recog ) return false;
	return addGrammar(name, dictfile, dfafile, true);
}

//============================
// Delete grammar by its name
//============================
bool cJulius::deleteGrammar( char *name )
{
	RecogProcess *r;
	int gid;

	if ( ! m_recog ) return false;

	r = m_recog->process_list;

	gid = multigram_get_id_by_name(r->lm, name);
	if (gid == -1) return false;

	if (multigram_delete(gid, r->lm) == FALSE) { /* deletion marking failed */
		return false;
	}
	/* need to rebuild the global lexicon */
	/* tell engine to update at requested timing */
	schedule_grammar_update(m_recog);

	PostMessage(getWindow(), WM_JULIUS, JEVENT_GRAM_UPDATE, 0L);

	return true;
}

//================================
// Deactivate grammar by its name
//================================
bool cJulius::deactivateGrammar( char *name )
{
	RecogProcess *r;
	int gid;
	int ret;

	if ( ! m_recog ) return false;

	r = m_recog->process_list;

	gid = multigram_get_id_by_name(r->lm, name);
	if (gid == -1) return false;

	ret = multigram_deactivate(gid, r->lm);
	if (ret == 1) {
		/* already active */
		return true;
	} else if (ret == -1) {
		/* not found */
		return false;
	}
	/* tell engine to update at requested timing */
	schedule_grammar_update(m_recog);
	PostMessage(getWindow(), WM_JULIUS, JEVENT_GRAM_UPDATE, 0L);

	return true;
}

//=================================
// Re-activate grammar by its name
//=================================
bool cJulius::activateGrammar( char *name )
{
	RecogProcess *r;
	int gid;
	int ret;

	if ( ! m_recog ) return false;

	r = m_recog->process_list;

	gid = multigram_get_id_by_name(r->lm, name);
	if (gid == -1) return false;

	ret = multigram_activate(gid, r->lm);
	if (ret == 1) {
		/* already active */
		return true;
	} else if (ret == -1) {
		/* not found */
		return false;
	}
	/* tell engine to update at requested timing */
	schedule_grammar_update(m_recog);
	PostMessage(getWindow(), WM_JULIUS, JEVENT_GRAM_UPDATE, 0L);

	return true;
}

//=====================================
// Stop processes and release all data
//=====================================
void cJulius::release( void )
{

	if ( ! m_recog ) return;

	stopProcess();

	if (m_threadHandle) {
		CloseHandle(m_threadHandle);
		m_threadHandle = NULL;
	} else {
		if ( m_recog ) {
			j_recog_free( m_recog );  // jconf will be released inside this
			m_recog = NULL;
			m_jconf = NULL;
		}
		if ( m_jconf ) {
			j_jconf_free( m_jconf );
			m_jconf = NULL;
		}
	}

}
