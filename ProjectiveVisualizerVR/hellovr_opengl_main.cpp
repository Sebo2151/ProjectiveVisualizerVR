//========= Copyright Valve Corporation ============//



#include <SDL.h>
#include <GL/glew.h>
#include <SDL_opengl.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <Windows.h>

#if defined( OSX )
#include <Foundation/Foundation.h>
#include <AppKit/AppKit.h>
#include <OpenGL/glu.h>
// Apple's version of glut.h #undef's APIENTRY, redefine it
#define APIENTRY
#else
#include <GL/glu.h>
#endif
#include <stdio.h>
#include <string>
#include <sstream>
#include <cstdlib>
#include <iostream>

#include <openvr.h>

#include "shared/lodepng.h"
#include "shared/Matrices.h"
#include "shared/pathtools.h"

#include "term.h"
#include "functionmesh.h"

#if defined(POSIX)
#include "unistd.h"
#endif

#ifndef _WIN32
#define APIENTRY
#endif

#ifndef _countof
#define _countof(x) (sizeof(x)/sizeof((x)[0]))
#endif

void ThreadSleep( unsigned long nMilliseconds )
{
#if defined(_WIN32)
	::Sleep( nMilliseconds );
#elif defined(POSIX)
	usleep( nMilliseconds * 1000 );
#endif
}

class CGLRenderModel
{
public:
	CGLRenderModel( const std::string & sRenderModelName );
	~CGLRenderModel();

	bool BInit( const vr::RenderModel_t & vrModel, const vr::RenderModel_TextureMap_t & vrDiffuseTexture );
	void Cleanup();
	void Draw();
	const std::string & GetName() const { return m_sModelName; }

private:
	GLuint m_glVertBuffer;
	GLuint m_glIndexBuffer;
	GLuint m_glVertArray;
	GLuint m_glTexture;
	GLsizei m_unVertexCount;
	std::string m_sModelName;
};



class FunctionTextInput
{
public:
	FunctionTextInput(std::string initial_string = "")
	{
		set_str(initial_string); // Currently no check for correctness.
	}

	~FunctionTextInput()
	{

	}

	void type_char(char c)
	{
		str.insert(cursor_pos, 1, c);
		cursor_pos++;
	}
	void delete_char()
	{
		if (cursor_pos > 0)
		{
			str.erase(cursor_pos - 1, 1);
			cursor_pos--;
		}
	}
	void cursor_right()
	{
		cursor_pos = (cursor_pos + 1 > str.length() ? str.length() : cursor_pos + 1);
	}
	void cursor_left()
	{
		cursor_pos = (cursor_pos - 1 > 0 ? cursor_pos - 1 : 0);
	}

	const std::string get_str()
	{
		return str;
	}

	void set_str(std::string new_str)
	{
		str = new_str;
		cursor_pos = str.length();
	}

	void set_pose(Matrix4 hmd_pose)
	{
		pose = hmd_pose; // Will actually need some adjustment.
	}

	void render(FT_Face robotoFace)
	{
		// First, choose dimensions
		// Then render text to texture
		// Then render a 3D object with that texture.
	}

	std::string str;
	int cursor_pos;

	GLuint glVertBuffer;
	GLuint glIndexBuffer;
	GLuint glVAO;

	GLuint glFramebufferId;
	GLuint glTextureId;

	GLuint glCharVertBuffer;
	GLuint glCharIndexBuffer;
	GLuint glCharVAO;
	GLuint glCharTextures[256]; // The size of ASCII alphabet

	const int texture_width = 2048;
	const int texture_height = 2048;
	
	const int char_texture_height = 256;

	Matrix4 pose;

	bool is_correct = true;
	Uint32 ticks_when_last_valid = 0;
};

static bool g_bPrintf = true;


//-----------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
class CMainApplication
{
public:
	CMainApplication( int argc, char *argv[] );
	virtual ~CMainApplication();

	bool BInit();
	bool BInitGL();
	bool BInitCompositor();

	void SetupRenderModels();

	void Shutdown();

	void RunMainLoop();
	bool HandleInput();
	void ProcessVREvent( const vr::VREvent_t & event );
	void RenderFrame();

	bool SetupFunctionTexture();

	void RenderControllerAxes();

	bool SetupStereoRenderTargets();
	void SetupCompanionWindow();
	void SetupCameras();

	void SetupFunction();
	void AsynchReplaceFunction();
	void CreateAsynchReplaceFunctionThread();

	void SetupFunctionTextInput();

	void RenderStereoTargets();
	void RenderCompanionWindow();
	void RenderScene( vr::Hmd_Eye nEye );
	void RenderFunction(vr::Hmd_Eye nEye);
	void RenderFunctionTextInput(vr::Hmd_Eye nEye);

	Matrix4 GetHMDMatrixProjectionEye( vr::Hmd_Eye nEye );
	Matrix4 GetHMDMatrixPoseEye( vr::Hmd_Eye nEye );
	Matrix4 GetCurrentViewProjectionMatrix( vr::Hmd_Eye nEye );
	void UpdateHMDMatrixPose();

	Matrix4 ConvertSteamVRMatrixToMatrix4( const vr::HmdMatrix34_t &matPose );

	GLuint CompileGLShader( const char *pchShaderName, const char *pchVertexShader, const char *pchFragmentShader );
	bool CreateAllShaders();

	void SetupRenderModelForTrackedDevice( vr::TrackedDeviceIndex_t unTrackedDeviceIndex );
	CGLRenderModel *FindOrLoadRenderModel( const char *pchRenderModelName );

private: 
	bool m_bDebugOpenGL;
	bool m_bVerbose;
	bool m_bPerf;
	bool m_bVblank;
	bool m_bGlFinishHack;

	vr::IVRSystem *m_pHMD;
	vr::IVRRenderModels *m_pRenderModels;
	std::string m_strDriver;
	std::string m_strDisplay;
	vr::TrackedDevicePose_t m_rTrackedDevicePose[ vr::k_unMaxTrackedDeviceCount ];
	Matrix4 m_rmat4DevicePose[ vr::k_unMaxTrackedDeviceCount ];
	bool m_rbShowTrackedDevice[ vr::k_unMaxTrackedDeviceCount ];

	Term* m_function;
	FunctionMesh* m_functionMesh;
	Term* m_functionUnderConstruction;
	FunctionMesh* m_FunctionMeshUnderConstruction;
	bool m_bFunctionMeshIsUnderConstruction;
	FunctionTextInput m_functionTextInput;

	FT_Library m_ftLibrary;
	FT_Face m_robotoFace;

	// Input handling
	Matrix4 m_triggerPressedPoseInverse;
	Matrix4 m_fromTriggerPressedPose;
	Matrix4 m_functionPose;
	bool m_bTriggerIsHeld;

	Vector4 m_rotationStartPos;
	Matrix4 m_temporaryRotation;
	bool m_bRotatingThroughInfinity;

	bool m_bInMenu;

	unsigned int m_nActiveControllerID;

private: // SDL bookkeeping
	SDL_Window *m_pCompanionWindow;
	uint32_t m_nCompanionWindowWidth;
	uint32_t m_nCompanionWindowHeight;

	SDL_GLContext m_pContext;

private: // OpenGL bookkeeping
	int m_iTrackedControllerCount;
	int m_iTrackedControllerCount_Last;
	int m_iValidPoseCount;
	int m_iValidPoseCount_Last;

	std::string m_strPoseClasses;                            // what classes we saw poses for this frame
	char m_rDevClassChar[ vr::k_unMaxTrackedDeviceCount ];   // for each device, a character representing its class
	
	float m_fNearClip;
	float m_fFarClip;

	GLuint m_nFunctionTexture;

	GLuint m_glSceneVertBuffer;
	GLuint m_unSceneVAO;
	GLuint m_unCompanionWindowVAO;
	GLuint m_glCompanionWindowIDVertBuffer;
	GLuint m_glCompanionWindowIDIndexBuffer;
	unsigned int m_uiCompanionWindowIndexSize;

	GLuint m_glControllerVertBuffer;
	GLuint m_unControllerVAO;
	unsigned int m_uiControllerVertcount;

	Matrix4 m_mat4HMDPose;
	Matrix4 m_mat4eyePosLeft;
	Matrix4 m_mat4eyePosRight;

	Matrix4 m_mat4ProjectionCenter;
	Matrix4 m_mat4ProjectionLeft;
	Matrix4 m_mat4ProjectionRight;

	struct VertexDataScene
	{
		Vector3 position;
		Vector2 texCoord;

		VertexDataScene(const Vector3 & pos, const Vector2 tex) : position(pos), texCoord(tex) {}
	};

	struct VertexDataWindow
	{
		Vector2 position;
		Vector2 texCoord;

		VertexDataWindow( const Vector2 & pos, const Vector2 tex ) :  position(pos), texCoord(tex) {	}
	};

	GLuint m_unSceneProgramID;
	GLuint m_unCompanionWindowProgramID;
	GLuint m_unControllerTransformProgramID;
	GLuint m_unRenderModelProgramID;

	GLuint m_unFunctionProgramID;

	GLint m_nSceneMatrixLocation;
	GLint m_nControllerMatrixLocation;
	GLint m_nRenderModelMatrixLocation;

	GLuint m_functionVertBuffer;
	GLuint m_functionNormalBuffer;
	GLuint m_functionVAO;

	bool m_bDebugCubes;
	GLuint m_debugCubesVertBuffer;
	GLuint m_debugCubesColorBuffer;
	GLuint m_debugCubesVAO;

	GLint m_nFunctionMatrixLocation;
	GLint m_nFunctionSurfaceColorLocation;

	struct FramebufferDesc
	{
		GLuint m_nDepthBufferId;
		GLuint m_nRenderTextureId;
		GLuint m_nRenderFramebufferId;
		GLuint m_nResolveTextureId;
		GLuint m_nResolveFramebufferId;
	};
	FramebufferDesc leftEyeDesc;
	FramebufferDesc rightEyeDesc;

	bool CreateFrameBuffer( int nWidth, int nHeight, FramebufferDesc &framebufferDesc );
	
	uint32_t m_nRenderWidth;
	uint32_t m_nRenderHeight;

	std::vector< CGLRenderModel * > m_vecRenderModels;
	CGLRenderModel *m_rTrackedDeviceToRenderModel[ vr::k_unMaxTrackedDeviceCount ];
};

//-----------------------------------------------------------------------------
// Purpose: Outputs a set of optional arguments to debugging output, using
//          the printf format setting specified in fmt*.
//-----------------------------------------------------------------------------
void dprintf( const char *fmt, ... )
{
	va_list args;
	char buffer[ 2048 ];

	va_start( args, fmt );
	vsprintf_s( buffer, fmt, args );
	va_end( args );

	if ( g_bPrintf )
		printf( "%s", buffer );

	OutputDebugStringA( buffer );
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CMainApplication::CMainApplication( int argc, char *argv[] )
	: m_pCompanionWindow(NULL)
	, m_pContext(NULL)
	, m_nCompanionWindowWidth( 2160 ) // Previously 640 x 320
	, m_nCompanionWindowHeight( 1200 )
	, m_unSceneProgramID( 0 )
	, m_unCompanionWindowProgramID( 0 )
	, m_unControllerTransformProgramID( 0 )
	, m_unRenderModelProgramID( 0 )
	, m_pHMD( NULL )
	, m_pRenderModels( NULL )
	, m_bDebugOpenGL( false )
	, m_bVerbose( false )
	, m_bPerf( false )
	, m_bVblank( false )
	, m_bGlFinishHack( true )
	, m_glControllerVertBuffer( 0 )
	, m_unControllerVAO( 0 )
	, m_unSceneVAO( 0 )
	, m_nSceneMatrixLocation( -1 )
	, m_nControllerMatrixLocation( -1 )
	, m_nRenderModelMatrixLocation( -1 )
	, m_iTrackedControllerCount( 0 )
	, m_iTrackedControllerCount_Last( -1 )
	, m_iValidPoseCount( 0 )
	, m_iValidPoseCount_Last( -1 )
	, m_strPoseClasses("")
	, m_function(NULL)
	, m_functionMesh(NULL)
	, m_functionVAO(0)
	, m_bTriggerIsHeld(false)
	, m_bInMenu(false)
	, m_nActiveControllerID(-1)
	, m_bDebugCubes( false )
	, m_FunctionMeshUnderConstruction( NULL )
	, m_bFunctionMeshIsUnderConstruction( false )
	, m_functionUnderConstruction( NULL )
	, m_ftLibrary( NULL )
	, m_robotoFace( NULL )
{

	for( int i = 1; i < argc; i++ )
	{
		if( !stricmp( argv[i], "-gldebug" ) )
		{
			m_bDebugOpenGL = true;
		}
		else if( !stricmp( argv[i], "-verbose" ) )
		{
			m_bVerbose = true;
		}
		else if( !stricmp( argv[i], "-novblank" ) )
		{
			m_bVblank = false;
		}
		else if( !stricmp( argv[i], "-noglfinishhack" ) )
		{
			m_bGlFinishHack = false;
		}
		else if( !stricmp( argv[i], "-noprintf" ) )
		{
			g_bPrintf = false;
		}
	}
	// other initialization tasks are done in BInit
	memset(m_rDevClassChar, 0, sizeof(m_rDevClassChar));
};


//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CMainApplication::~CMainApplication()
{
	// work is done in Shutdown
	dprintf( "Shutdown" );
}


//-----------------------------------------------------------------------------
// Purpose: Helper to get a string from a tracked device property and turn it
//			into a std::string
//-----------------------------------------------------------------------------
std::string GetTrackedDeviceString( vr::IVRSystem *pHmd, vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError *peError = NULL )
{
	uint32_t unRequiredBufferLen = pHmd->GetStringTrackedDeviceProperty( unDevice, prop, NULL, 0, peError );
	if( unRequiredBufferLen == 0 )
		return "";

	char *pchBuffer = new char[ unRequiredBufferLen ];
	unRequiredBufferLen = pHmd->GetStringTrackedDeviceProperty( unDevice, prop, pchBuffer, unRequiredBufferLen, peError );
	std::string sResult = pchBuffer;
	delete [] pchBuffer;
	return sResult;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CMainApplication::BInit()
{
	if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_TIMER ) < 0 )
	{
		printf("%s - SDL could not initialize! SDL Error: %s\n", __FUNCTION__, SDL_GetError());
		return false;
	}

	// Loading the SteamVR Runtime
	vr::EVRInitError eError = vr::VRInitError_None;
	m_pHMD = vr::VR_Init( &eError, vr::VRApplication_Scene );

	if ( eError != vr::VRInitError_None )
	{
		m_pHMD = NULL;
		char buf[1024];
		sprintf_s( buf, sizeof( buf ), "Unable to init VR runtime: %s", vr::VR_GetVRInitErrorAsEnglishDescription( eError ) );
		SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, "VR_Init Failed", buf, NULL );
		return false;
	}


	m_pRenderModels = (vr::IVRRenderModels *)vr::VR_GetGenericInterface( vr::IVRRenderModels_Version, &eError );
	if( !m_pRenderModels )
	{
		m_pHMD = NULL;
		vr::VR_Shutdown();

		char buf[1024];
		sprintf_s( buf, sizeof( buf ), "Unable to get render model interface: %s", vr::VR_GetVRInitErrorAsEnglishDescription( eError ) );
		SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, "VR_Init Failed", buf, NULL );
		return false;
	}

	int nWindowPosX = 700;
	int nWindowPosY = 100;
	Uint32 unWindowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;

	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 4 );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 1 );
	//SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );

	SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, 0 );
	SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, 0 );
	if( m_bDebugOpenGL )
		SDL_GL_SetAttribute( SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG );

	m_pCompanionWindow = SDL_CreateWindow( "Projective Viewer VR", nWindowPosX, nWindowPosY, m_nCompanionWindowWidth, m_nCompanionWindowHeight, unWindowFlags );
	if (m_pCompanionWindow == NULL)
	{
		printf( "%s - Window could not be created! SDL Error: %s\n", __FUNCTION__, SDL_GetError() );
		return false;
	}

	m_pContext = SDL_GL_CreateContext(m_pCompanionWindow);
	if (m_pContext == NULL)
	{
		printf( "%s - OpenGL context could not be created! SDL Error: %s\n", __FUNCTION__, SDL_GetError() );
		return false;
	}

	glewExperimental = GL_TRUE;
	GLenum nGlewError = glewInit();
	if (nGlewError != GLEW_OK)
	{
		printf( "%s - Error initializing GLEW! %s\n", __FUNCTION__, glewGetErrorString( nGlewError ) );
		return false;
	}
	glGetError(); // to clear the error caused deep in GLEW

	if ( SDL_GL_SetSwapInterval( m_bVblank ? 1 : 0 ) < 0 )
	{
		printf( "%s - Warning: Unable to set VSync! SDL Error: %s\n", __FUNCTION__, SDL_GetError() );
		return false;
	}


	m_strDriver = "No Driver";
	m_strDisplay = "No Display";

	m_strDriver = GetTrackedDeviceString( m_pHMD, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_TrackingSystemName_String );
	m_strDisplay = GetTrackedDeviceString( m_pHMD, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_SerialNumber_String );

	std::string strWindowTitle = "Projective Viewer VR - " + m_strDriver + " " + m_strDisplay;
	SDL_SetWindowTitle( m_pCompanionWindow, strWindowTitle.c_str() );
 		
 	m_fNearClip = 0.1f;
 	m_fFarClip = 30.0f;

	
	if (!BInitGL())
	{
		printf("%s - Unable to initialize OpenGL!\n", __FUNCTION__);
		return false;
	}

	if (!BInitCompositor())
	{
		printf("%s - Failed to initialize VR Compositor!\n", __FUNCTION__);
		return false;
	}

	SetupFunction();
	SetupFunctionTextInput();

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Outputs the string in message to debugging output.
//          All other parameters are ignored.
//          Does not return any meaningful value or reference.
//-----------------------------------------------------------------------------
void APIENTRY DebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const char* message, const void* userParam)
{
	dprintf( "GL Error: %s\n", message );
}


//-----------------------------------------------------------------------------
// Purpose: Initialize OpenGL. Returns true if OpenGL has been successfully
//          initialized, false if shaders could not be created.
//          If failure occurred in a module other than shaders, the function
//          may return true or throw an error. 
//-----------------------------------------------------------------------------
bool CMainApplication::BInitGL()
{
	if( m_bDebugOpenGL )
	{
		glDebugMessageCallback( (GLDEBUGPROC)DebugCallback, nullptr);
		glDebugMessageControl( GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE );
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	}

	if( !CreateAllShaders() )
		return false;

	SetupFunctionTexture();
	SetupCameras();
	SetupStereoRenderTargets();
	SetupCompanionWindow();
	SetupRenderModels();

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Initialize Compositor. Returns true if the compositor was
//          successfully initialized, false otherwise.
//-----------------------------------------------------------------------------
bool CMainApplication::BInitCompositor()
{
	if ( !vr::VRCompositor() )
	{
		printf( "Compositor initialization failed. See log file for details\n" );
		return false;
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMainApplication::Shutdown()
{
	if( m_pHMD )
	{
		vr::VR_Shutdown();
		m_pHMD = NULL;
	}

	for( std::vector< CGLRenderModel * >::iterator i = m_vecRenderModels.begin(); i != m_vecRenderModels.end(); i++ )
	{
		delete (*i);
	}
	m_vecRenderModels.clear();
	
	if( m_pContext )
	{
		if( m_bDebugOpenGL )
		{
			glDebugMessageControl( GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_FALSE );
			glDebugMessageCallback(nullptr, nullptr);
		}
		glDeleteBuffers(1, &m_glSceneVertBuffer);

		if ( m_unSceneProgramID )
		{
			glDeleteProgram( m_unSceneProgramID );
		}
		if ( m_unControllerTransformProgramID )
		{
			glDeleteProgram( m_unControllerTransformProgramID );
		}
		if ( m_unRenderModelProgramID )
		{
			glDeleteProgram( m_unRenderModelProgramID );
		}
		if ( m_unCompanionWindowProgramID )
		{
			glDeleteProgram( m_unCompanionWindowProgramID );
		}

		glDeleteRenderbuffers( 1, &leftEyeDesc.m_nDepthBufferId );
		glDeleteTextures( 1, &leftEyeDesc.m_nRenderTextureId );
		glDeleteFramebuffers( 1, &leftEyeDesc.m_nRenderFramebufferId );
		glDeleteTextures( 1, &leftEyeDesc.m_nResolveTextureId );
		glDeleteFramebuffers( 1, &leftEyeDesc.m_nResolveFramebufferId );

		glDeleteRenderbuffers( 1, &rightEyeDesc.m_nDepthBufferId );
		glDeleteTextures( 1, &rightEyeDesc.m_nRenderTextureId );
		glDeleteFramebuffers( 1, &rightEyeDesc.m_nRenderFramebufferId );
		glDeleteTextures( 1, &rightEyeDesc.m_nResolveTextureId );
		glDeleteFramebuffers( 1, &rightEyeDesc.m_nResolveFramebufferId );

		if( m_unCompanionWindowVAO != 0 )
		{
			glDeleteVertexArrays( 1, &m_unCompanionWindowVAO );
		}
		if( m_unSceneVAO != 0 )
		{
			glDeleteVertexArrays( 1, &m_unSceneVAO );
		}
		if( m_unControllerVAO != 0 )
		{
			glDeleteVertexArrays( 1, &m_unControllerVAO );
		}
		if (m_functionVAO != 0)
		{
			glDeleteVertexArrays(1, &m_functionVAO);
		}
	}

	if( m_pCompanionWindow )
	{
		SDL_DestroyWindow(m_pCompanionWindow);
		m_pCompanionWindow = NULL;
	}

	SDL_Quit();

	if (m_function != 0)
	{
		delete m_function;
	}
	if (m_functionMesh != 0)
	{
		delete m_functionMesh;

		glDeleteBuffers(1, &m_functionVertBuffer);
		glDeleteBuffers(1, &m_functionNormalBuffer);
	}
}

Matrix4 generateRotationThroughInfinity(Vector4 start, Vector4 stop)
{
	// Black magic below. The idea is to extend the rotation in the plane spanned by start and stop
	// that takes start to a multiple of stop to all of R^4 in an orthogonal way.

	Vector4 v = start.normalizeAll();
	Vector4 w = stop.normalizeAll();

	float cosTheta = v.dot(w);
	float sinTheta = sqrtf(1 - cosTheta*cosTheta);



	// In case we've moved so little that we might get numerical problems, say we haven't rotated at all.
	Vector4 vw_diff = v - w;
	if (vw_diff.length() < 0.001)
		return Matrix4();


	// Graham-Schmidt.
	// The idea is to take the two largest-norm Graham-Schmidtified vectors to form an orthonormal basis.
	// It's possible for some of the e_is to be in or nearly in the span of v and w, so it's necessary
	// to avoid using vectors of small norm.

	w = w - w.dot(v) * v;
	w.normalizeAll();

	
	Vector4 ei_proj[4] = { Vector4(1,0,0,0),
						   Vector4(0,1,0,0),
						   Vector4(0,0,1,0),
						   Vector4(0,0,0,1) };

	// Subtract the v, w components.
	for (int i = 0; i < 4; i++)
	{
		ei_proj[i] = ei_proj[i] - ei_proj[i].dot(v) * v;
		ei_proj[i] = ei_proj[i] - ei_proj[i].dot(w) * w;
	}

	// Select the vector of largest remaining norm.
	int max_norm_i = 0;
	float max_norm = ei_proj[0].length();
	for (int i = 1; i < 4; i++)
	{
		if (ei_proj[i].length() > max_norm)
		{
			max_norm_i = i;
			max_norm = ei_proj[i].length();
		}
	}

	ei_proj[max_norm_i].normalizeAll();

	// Subtract the ei_proj[max_norm_i] components...
	for (int i = 0; i < 4; i++)
	{
		if (i == max_norm_i)
			continue;
		else
			ei_proj[i] = ei_proj[i] - ei_proj[i].dot(ei_proj[max_norm_i]) * ei_proj[max_norm_i];
	}

	// Select last basis vector, dodging the one we already picked...
	int max_norm_j = (max_norm_i == 0) ? 1 : 0;
	max_norm = ei_proj[max_norm_j].length();
	for (int j = (max_norm_i == 0) ? 2 : 1; j < 4; j++)
	{
		if (j == max_norm_i)
			continue;
		else if (ei_proj[j].length() > max_norm)
		{
			max_norm_j = j;
			max_norm = ei_proj[j].length();
		}
	}

	ei_proj[max_norm_j].normalizeAll();

	/*
	int max_norm_i = 2;
	int max_norm_j = 3;


	ei_proj[2] = ei_proj[2] - ei_proj[2].dot(v) * v - ei_proj[2].dot(w) * w;
	ei_proj[2].normalizeAll();

	ei_proj[3] = ei_proj[3] - ei_proj[3].dot(v) * v - ei_proj[3].dot(w) * w - ei_proj[3].dot(ei_proj[2]) * ei_proj[2];
	ei_proj[3].normalizeAll();
	*/

	// At this point we have selected an orthonormal basis, with respect to which our transformation is quite simple.

	Matrix4 B = Matrix4(
		v.x, v.y, v.z, v.w,
		w.x, w.y, w.z, w.w,
		ei_proj[max_norm_i].x, ei_proj[max_norm_i].y, ei_proj[max_norm_i].z, ei_proj[max_norm_i].w,
		ei_proj[max_norm_j].x, ei_proj[max_norm_j].y, ei_proj[max_norm_j].z, ei_proj[max_norm_j].w
	);

	Matrix4 B_inv = B;
	B_inv.invert();

	Matrix4 R = Matrix4(
		cosTheta, sinTheta, 0, 0,
		-sinTheta, cosTheta, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	);

	Matrix4 A = B* R * B_inv;

	/*
	dprintf("rotating from (%f, %f, %f, %f) -> (%f, %f, %f, %f) is done by:\n", v.x, v.w, v.y, v.z, w.x, w.y, w.z, w.w);

	dprintf("%f %f %f %f\n%f %f %f %f\n%f %f %f %f\n%f %f %f %f\n",
		A.get()[0], A.get()[4], A.get()[8], A.get()[12],
		A.get()[1], A.get()[5], A.get()[9], A.get()[13],
		A.get()[2], A.get()[6], A.get()[10], A.get()[14],
		A.get()[3], A.get()[7], A.get()[11], A.get()[15]);

	dprintf("and the basis thing is:\n");

	dprintf("%f %f %f %f\n%f %f %f %f\n%f %f %f %f\n%f %f %f %f\n",
		B.get()[0], B.get()[4], B.get()[8], B.get()[12],
		B.get()[1], B.get()[5], B.get()[9], B.get()[13],
		B.get()[2], B.get()[6], B.get()[10], B.get()[14],
		B.get()[3], B.get()[7], B.get()[11], B.get()[15]);
		*/

	return A;
}

// Function to pass to Windows threading for AsynchReplaceFunction stuff.
DWORD WINAPI AsynchReplaceFunctionStarter(LPVOID vparams)
{
	CMainApplication* main_app = (CMainApplication*)vparams;

	main_app->AsynchReplaceFunction();

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CMainApplication::HandleInput()
{
	SDL_Event sdlEvent;
	bool bRet = false;

	while ( SDL_PollEvent( &sdlEvent ) != 0 )
	{
		if ( sdlEvent.type == SDL_QUIT )
		{
			bRet = true;
		}
		else if ( sdlEvent.type == SDL_KEYDOWN )
		{
			if ( sdlEvent.key.keysym.sym == SDLK_ESCAPE 
			     || sdlEvent.key.keysym.sym == SDLK_q )
			{
				bRet = true;
			}

			if (m_bInMenu)
			{
				// Handle Text Input
				switch (sdlEvent.key.keysym.sym)
				{
				case SDLK_LEFT:
					m_functionTextInput.cursor_left();
					break;
				case SDLK_RIGHT:
					m_functionTextInput.cursor_right();
					break;
				case SDLK_BACKSPACE:
					m_functionTextInput.delete_char();
					break;
				case SDLK_SPACE:
					m_functionTextInput.type_char(' ');
					break;
				case SDLK_x:
					m_functionTextInput.type_char('x');
					break;
				case SDLK_y:
					m_functionTextInput.type_char('y');
					break;
				case SDLK_z:
					m_functionTextInput.type_char('z');
					break;
				case SDLK_w:
					m_functionTextInput.type_char('w');
					break;
				case SDLK_KP_0:
					m_functionTextInput.type_char('0');
					break;
				case SDLK_0:
					if (sdlEvent.key.keysym.mod & KMOD_SHIFT)
						m_functionTextInput.type_char(')');
					else
						m_functionTextInput.type_char('0');
					break;
				case SDLK_KP_1:
				case SDLK_1:
					m_functionTextInput.type_char('1');
					break;
				case SDLK_KP_2:
				case SDLK_2:
					m_functionTextInput.type_char('2');
					break;
				case SDLK_KP_3:
				case SDLK_3:
					m_functionTextInput.type_char('3');
					break;
				case SDLK_KP_4:
				case SDLK_4:
					m_functionTextInput.type_char('4');
					break;
				case SDLK_KP_5:
				case SDLK_5:
					m_functionTextInput.type_char('5');
					break;
				case SDLK_KP_6:
					m_functionTextInput.type_char('6');
					break;
				case SDLK_6:
					if (sdlEvent.key.keysym.mod & KMOD_SHIFT)
						m_functionTextInput.type_char('^');
					else
						m_functionTextInput.type_char('6');
					break;
				case SDLK_KP_7:
				case SDLK_7:
					m_functionTextInput.type_char('7');
					break;
				case SDLK_KP_8:
					m_functionTextInput.type_char('8');
					break;
				case SDLK_8:
					if (sdlEvent.key.keysym.mod & KMOD_SHIFT)
						m_functionTextInput.type_char('*');
					else
						m_functionTextInput.type_char('8');
					break;
				case SDLK_KP_9:
					m_functionTextInput.type_char('9');
					break;
				case SDLK_9:
					if (sdlEvent.key.keysym.mod & KMOD_SHIFT)
						m_functionTextInput.type_char('(');
					else
						m_functionTextInput.type_char('9');
					break;
				case SDLK_EQUALS:
					if (sdlEvent.key.keysym.mod & KMOD_SHIFT)
					    m_functionTextInput.type_char('+');
					break;
				case SDLK_KP_PLUS:
					m_functionTextInput.type_char('+');
					break;
				case SDLK_KP_MINUS:
				case SDLK_MINUS:
                    m_functionTextInput.type_char('-');
					break;
				case SDLK_KP_MULTIPLY:
					m_functionTextInput.type_char('*');
					break;
				case SDLK_RETURN:
					if (!m_bInMenu)
					{
						// Only allow us to open menu if we're not already computing a mesh
						if (!m_bFunctionMeshIsUnderConstruction)
							m_bInMenu = true;
					}
					else
					{
						// Menu just closed. Try to use new function.
						m_bInMenu = false;

						CreateAsynchReplaceFunctionThread();
					}
					break;
				}
				dprintf("%s\n", m_functionTextInput.get_str().c_str());
			}
		}
	}

	// Process SteamVR events
	vr::VREvent_t event;
	while( m_pHMD->PollNextEvent( &event, sizeof( event ) ) )
	{
		ProcessVREvent( event );
	}

	static vr::VRControllerState_t prev_state[vr::k_unMaxTrackedDeviceCount];

	// Process SteamVR controller state
	for( vr::TrackedDeviceIndex_t unDevice = 0; unDevice < vr::k_unMaxTrackedDeviceCount; unDevice++ )
	{
		vr::VRControllerState_t state;
		if( m_pHMD->GetControllerState( unDevice, &state, sizeof(state) ) )
		{
			// Default behavior: disappear the device.
			// m_rbShowTrackedDevice[ unDevice ] = state.ulButtonPressed == 0;

			if (m_bTriggerIsHeld)
			{
				if (unDevice == m_nActiveControllerID)
				{
					if (!(state.ulButtonPressed & vr::ButtonMaskFromId(vr::k_EButton_SteamVR_Trigger)) || !m_rTrackedDevicePose[unDevice].bPoseIsValid)
					{
						// Consider trigger released; The transformation should now be permanently applied.
						m_bTriggerIsHeld = false;
						m_nActiveControllerID = -1;

						m_functionPose = m_rmat4DevicePose[unDevice] * m_triggerPressedPoseInverse * m_functionPose;
					}
					else
					{
						// Trigger is still held; update the transformation for temporary display.
						m_fromTriggerPressedPose = m_rmat4DevicePose[unDevice] * m_triggerPressedPoseInverse;
					}
				}
			}
			else if (m_bRotatingThroughInfinity)
			{
				if (unDevice == m_nActiveControllerID)
				{
					if (!(state.ulButtonPressed & vr::ButtonMaskFromId(vr::k_EButton_SteamVR_Touchpad)) || !m_rTrackedDevicePose[unDevice].bPoseIsValid)
					{
						// Consider rotation stopped. The rotation should now be permanently applied.
						m_bRotatingThroughInfinity = false;
						m_nActiveControllerID = -1;

						Vector4 m_rotationStopPos = Vector4(
							m_rmat4DevicePose[unDevice].get()[12],
							m_rmat4DevicePose[unDevice].get()[13],
							m_rmat4DevicePose[unDevice].get()[14],
							m_rmat4DevicePose[unDevice].get()[15]
						);

						m_functionPose = generateRotationThroughInfinity(m_rotationStartPos, m_rotationStopPos) * m_functionPose;
						m_temporaryRotation.identity();
					}
					else
					{
						Vector4 m_rotationStopPos = Vector4(
							m_rmat4DevicePose[unDevice].get()[12],
							m_rmat4DevicePose[unDevice].get()[13],
							m_rmat4DevicePose[unDevice].get()[14],
							m_rmat4DevicePose[unDevice].get()[15]
						);

						m_temporaryRotation = generateRotationThroughInfinity(m_rotationStartPos, m_rotationStopPos);
					}
				}
			}
			else
			{
				if ((state.ulButtonPressed & vr::ButtonMaskFromId(vr::k_EButton_SteamVR_Trigger)) && m_rTrackedDevicePose[unDevice].bPoseIsValid)
				{
					// Trigger is pressed. Start moving with the controller.

					m_nActiveControllerID = unDevice;
					m_triggerPressedPoseInverse = m_rmat4DevicePose[unDevice];
					m_triggerPressedPoseInverse.invert();
					m_fromTriggerPressedPose.identity();
					m_bTriggerIsHeld = true;
				}
				else if ((state.ulButtonPressed & vr::ButtonMaskFromId(vr::k_EButton_SteamVR_Touchpad)) && m_rTrackedDevicePose[unDevice].bPoseIsValid)
				{
					// Touchpad pressed. Start rotating through infinity.

					m_nActiveControllerID = unDevice;
					m_rotationStartPos = Vector4(
						m_rmat4DevicePose[unDevice].get()[12],
						m_rmat4DevicePose[unDevice].get()[13],
						m_rmat4DevicePose[unDevice].get()[14],
						m_rmat4DevicePose[unDevice].get()[15]
					);
					m_temporaryRotation.identity();
					m_bRotatingThroughInfinity = true;
				}
				else if (state.ulButtonPressed & vr::ButtonMaskFromId(vr::k_EButton_Grip))
				{
					// Reset function position
					m_functionPose = Matrix4().translate(0, 1, 0);
				}
				else if (state.ulButtonPressed & vr::ButtonMaskFromId(vr::k_EButton_ApplicationMenu)
					 && !(prev_state[unDevice].ulButtonPressed & vr::ButtonMaskFromId(vr::k_EButton_ApplicationMenu)))
				{
					if (!m_bInMenu)
					{
						// Only allow us to open menu if we're not already computing a mesh
						if (!m_bFunctionMeshIsUnderConstruction)
						    m_bInMenu = true;
					}
					else
					{
						// Menu just closed. Try to use new function.
						m_bInMenu = false;

						CreateAsynchReplaceFunctionThread();
					}
				}
			}

			prev_state[unDevice] = state;
		}
	}

	return bRet;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMainApplication::RunMainLoop()
{
	bool bQuit = false;

	SDL_StartTextInput();
	SDL_ShowCursor( SDL_DISABLE );

	while ( !bQuit )
	{
		UpdateHMDMatrixPose();

		bQuit = HandleInput();

		// An asynchronous function mesh computation has completed.
		if (!m_bFunctionMeshIsUnderConstruction && m_FunctionMeshUnderConstruction != NULL)
		{
			delete(m_functionMesh);
			m_functionMesh = m_FunctionMeshUnderConstruction;
			m_function = m_functionUnderConstruction;
			m_FunctionMeshUnderConstruction = NULL;
			m_functionUnderConstruction = NULL;
		}

		RenderFrame();
	}

	SDL_StopTextInput();
}


//-----------------------------------------------------------------------------
// Purpose: Processes a single VR event
//-----------------------------------------------------------------------------
void CMainApplication::ProcessVREvent( const vr::VREvent_t & event )
{
	switch( event.eventType )
	{
	case vr::VREvent_TrackedDeviceActivated:
		{
			SetupRenderModelForTrackedDevice( event.trackedDeviceIndex );
			dprintf( "Device %u attached. Setting up render model.\n", event.trackedDeviceIndex );
		}
		break;
	case vr::VREvent_TrackedDeviceDeactivated:
		{
			dprintf( "Device %u detached.\n", event.trackedDeviceIndex );
		}
		break;
	case vr::VREvent_TrackedDeviceUpdated:
		{
			dprintf( "Device %u updated.\n", event.trackedDeviceIndex );
		}
		break;
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMainApplication::RenderFrame()
{
	// for now as fast as possible
	if ( m_pHMD )
	{
		RenderControllerAxes();
		RenderStereoTargets();
		RenderCompanionWindow();

		vr::Texture_t leftEyeTexture = {(void*)(uintptr_t)leftEyeDesc.m_nResolveTextureId, vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
		vr::VRCompositor()->Submit(vr::Eye_Left, &leftEyeTexture );
		vr::Texture_t rightEyeTexture = {(void*)(uintptr_t)rightEyeDesc.m_nResolveTextureId, vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
		vr::VRCompositor()->Submit(vr::Eye_Right, &rightEyeTexture );
	}

	if ( m_bVblank && m_bGlFinishHack )
	{
		//$ HACKHACK. From gpuview profiling, it looks like there is a bug where two renders and a present
		// happen right before and after the vsync causing all kinds of jittering issues. This glFinish()
		// appears to clear that up. Temporary fix while I try to get nvidia to investigate this problem.
		// 1/29/2014 mikesart
		glFinish();
	}

	// SwapWindow
	{
		SDL_GL_SwapWindow( m_pCompanionWindow );
	}

	// Clear
	{
		// We want to make sure the glFinish waits for the entire present to complete, not just the submission
		// of the command. So, we do a clear here right here so the glFinish will wait fully for the swap.
		glClearColor( 1, 1, 1, 1 );
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	}

	// Flush and wait for swap.
	if ( m_bVblank )
	{
		glFlush();
		glFinish();
	}

	// Spew out the controller and pose count whenever they change.
	if ( m_iTrackedControllerCount != m_iTrackedControllerCount_Last || m_iValidPoseCount != m_iValidPoseCount_Last )
	{
		m_iValidPoseCount_Last = m_iValidPoseCount;
		m_iTrackedControllerCount_Last = m_iTrackedControllerCount;
		
		dprintf( "PoseCount:%d(%s) Controllers:%d\n", m_iValidPoseCount, m_strPoseClasses.c_str(), m_iTrackedControllerCount );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Compiles a GL shader program and returns the handle. Returns 0 if
//			the shader couldn't be compiled for some reason.
//-----------------------------------------------------------------------------
GLuint CMainApplication::CompileGLShader( const char *pchShaderName, const char *pchVertexShader, const char *pchFragmentShader )
{
	GLuint unProgramID = glCreateProgram();

	GLuint nSceneVertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource( nSceneVertexShader, 1, &pchVertexShader, NULL);
	glCompileShader( nSceneVertexShader );

	GLint vShaderCompiled = GL_FALSE;
	glGetShaderiv( nSceneVertexShader, GL_COMPILE_STATUS, &vShaderCompiled);
	if ( vShaderCompiled != GL_TRUE)
	{
		dprintf("%s - Unable to compile vertex shader %d!\n", pchShaderName, nSceneVertexShader);
		
		char err_string[256];
		err_string[255] = 0;
		int length;
		glGetShaderInfoLog(nSceneVertexShader, 256, &length, err_string);
		dprintf("%s\n", err_string);
		
		glDeleteProgram( unProgramID );
		glDeleteShader( nSceneVertexShader );
		return 0;
	}
	glAttachShader( unProgramID, nSceneVertexShader);
	glDeleteShader( nSceneVertexShader ); // the program hangs onto this once it's attached

	GLuint  nSceneFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource( nSceneFragmentShader, 1, &pchFragmentShader, NULL);
	glCompileShader( nSceneFragmentShader );

	GLint fShaderCompiled = GL_FALSE;
	glGetShaderiv( nSceneFragmentShader, GL_COMPILE_STATUS, &fShaderCompiled);
	if (fShaderCompiled != GL_TRUE)
	{
		dprintf("%s - Unable to compile fragment shader %d!\n", pchShaderName, nSceneFragmentShader );

		char err_string[256];
		err_string[255] = 0;
		int length;
		glGetShaderInfoLog(nSceneFragmentShader, 256, &length, err_string);
		dprintf("%s\n", err_string);

		glDeleteProgram( unProgramID );
		glDeleteShader( nSceneFragmentShader );
		return 0;	
	}

	glAttachShader( unProgramID, nSceneFragmentShader );
	glDeleteShader( nSceneFragmentShader ); // the program hangs onto this once it's attached

	glLinkProgram( unProgramID );

	GLint programSuccess = GL_TRUE;
	glGetProgramiv( unProgramID, GL_LINK_STATUS, &programSuccess);
	if ( programSuccess != GL_TRUE )
	{
		dprintf("%s - Error linking program %d!\n", pchShaderName, unProgramID);
		glDeleteProgram( unProgramID );
		return 0;
	}

	glUseProgram( unProgramID );
	glUseProgram( 0 );

	return unProgramID;
}


//-----------------------------------------------------------------------------
// Purpose: Creates all the shaders used by HelloVR SDL
//-----------------------------------------------------------------------------
bool CMainApplication::CreateAllShaders()
{
	m_unSceneProgramID = CompileGLShader( 
		"Scene",

		// Vertex Shader
		"#version 410\n"
		"uniform mat4 matrix;\n"
		"layout(location = 0) in vec4 position;\n"
		"layout(location = 1) in vec2 v2UVcoordsIn;\n"
		"layout(location = 2) in vec3 v3NormalIn;\n"
		"out vec2 v2UVcoords;\n"
		"void main()\n"
		"{\n"
		"	v2UVcoords = v2UVcoordsIn;\n"
		"	gl_Position = matrix * position;\n"
		"}\n",

		// Fragment Shader
		"#version 410 core\n"
		"uniform sampler2D mytexture;\n"
		"in vec2 v2UVcoords;\n"
		"out vec4 outputColor;\n"
		"void main()\n"
		"{\n"
		"   outputColor = texture(mytexture, v2UVcoords);\n"
		"}\n"
		);
	m_nSceneMatrixLocation = glGetUniformLocation( m_unSceneProgramID, "matrix" );
	if( m_nSceneMatrixLocation == -1 )
	{
		dprintf( "Unable to find matrix uniform in scene shader\n" );
		return false;
	}

	m_unControllerTransformProgramID = CompileGLShader(
		"Controller",

		// vertex shader
		"#version 410\n"
		"uniform mat4 matrix;\n"
		"layout(location = 0) in vec4 position;\n"
		"layout(location = 1) in vec3 v3ColorIn;\n"
		"out vec4 v4Color;\n"
		"void main()\n"
		"{\n"
		"	v4Color.xyz = v3ColorIn; v4Color.a = 1.0;\n"
		"	gl_Position = matrix * position;\n"
		"}\n",

		// fragment shader
		"#version 410\n"
		"in vec4 v4Color;\n"
		"out vec4 outputColor;\n"
		"void main()\n"
		"{\n"
		"   outputColor = v4Color;\n"
		"}\n"
		);
	m_nControllerMatrixLocation = glGetUniformLocation( m_unControllerTransformProgramID, "matrix" );
	if( m_nControllerMatrixLocation == -1 )
	{
		dprintf( "Unable to find matrix uniform in controller shader\n" );
		return false;
	}

	m_unRenderModelProgramID = CompileGLShader( 
		"render model",

		// vertex shader
		"#version 410\n"
		"uniform mat4 matrix;\n"
		"layout(location = 0) in vec4 position;\n"
		"layout(location = 1) in vec3 v3NormalIn;\n"
		"layout(location = 2) in vec2 v2TexCoordsIn;\n"
		"out vec2 v2TexCoord;\n"
		"void main()\n"
		"{\n"
		"	v2TexCoord = v2TexCoordsIn;\n"
		"	gl_Position = matrix * vec4(position.xyz, 1);\n"
		"}\n",

		//fragment shader
		"#version 410 core\n"
		"uniform sampler2D diffuse;\n"
		"in vec2 v2TexCoord;\n"
		"out vec4 outputColor;\n"
		"void main()\n"
		"{\n"
		"   outputColor = texture( diffuse, v2TexCoord);\n"
		"}\n"

		);
	m_nRenderModelMatrixLocation = glGetUniformLocation( m_unRenderModelProgramID, "matrix" );
	if( m_nRenderModelMatrixLocation == -1 )
	{
		dprintf( "Unable to find matrix uniform in render model shader\n" );
		return false;
	}

	m_unCompanionWindowProgramID = CompileGLShader(
		"CompanionWindow",

		// vertex shader
		"#version 410 core\n"
		"layout(location = 0) in vec4 position;\n"
		"layout(location = 1) in vec2 v2UVIn;\n"
		"noperspective out vec2 v2UV;\n"
		"void main()\n"
		"{\n"
		"	v2UV = v2UVIn;\n"
		"	gl_Position = position;\n"
		"}\n",

		// fragment shader
		"#version 410 core\n"
		"uniform sampler2D mytexture;\n"
		"noperspective in vec2 v2UV;\n"
		"out vec4 outputColor;\n"
		"void main()\n"
		"{\n"
		"		outputColor = texture(mytexture, v2UV);\n"
		"}\n"
		);

	m_unFunctionProgramID = CompileGLShader(
		"function mesh",

		// vertex shaderc
		"#version 410 core\n"
		"layout(location = 0) in vec4 vertexPosition_modelspace;\n"
		"layout(location = 1) in vec3 vertexNormal;\n"
		"uniform mat4 projection;\n"
		"uniform vec3 surfaceColor;\n"
		"out vec3 fragmentColor;\n"
		"out vec3 texCoordsOut;\n"
		"void main(void)\n"
		"{\n"
		"	gl_Position = projection*vertexPosition_modelspace;\n"
		"	fragmentColor = surfaceColor * abs(dot(vertexNormal, vec3(0.4,0.7,0.5))) + 0.003*dot(vertexPosition_modelspace, vertexPosition_modelspace);\n"
		"   texCoordsOut = vec3(vertexPosition_modelspace);\n"
		"}\n",

		// fragment shader
		"#version 410 core\n"
		"uniform sampler3D myTexture;\n"
		"in vec3 fragmentColor;\n"
		"in vec3 texCoordsOut;\n"
		"out vec3 color;\n"
		"void main(void)\n"
		"{\n"
		"    color = vec3(texture(myTexture, texCoordsOut))*fragmentColor;\n"
		"}\n"
	);

	m_nFunctionMatrixLocation = glGetUniformLocation(m_unFunctionProgramID, "projection");
	m_nFunctionSurfaceColorLocation = glGetUniformLocation(m_unFunctionProgramID, "surfaceColor");

	if (m_nFunctionMatrixLocation == -1)
	{
		dprintf("Unable to find matrix uniform in function mesh shader\n");
		return false;
	}

	return m_unSceneProgramID != 0 
		&& m_unControllerTransformProgramID != 0
		&& m_unRenderModelProgramID != 0
		&& m_unCompanionWindowProgramID != 0
		&& m_unFunctionProgramID;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CMainApplication::SetupFunctionTexture()
{
	// Set up texture for surface

	glGenTextures(1, &m_nFunctionTexture);
	glEnable(GL_TEXTURE_3D);
	glBindTexture(GL_TEXTURE_3D, m_nFunctionTexture);

	const int fn_tex_size = 40;
	unsigned char fn_texture_data[fn_tex_size][fn_tex_size][fn_tex_size][3];

	for (int i = 0; i < fn_tex_size; i++)
	{
		for (int j = 0; j < fn_tex_size; j++)
		{
			for (int k = 0; k < fn_tex_size; k++)
			{
				fn_texture_data[i][j][k][0] = (i == 0 || j == 0 || k == 0) ? 100 : 255;
				fn_texture_data[i][j][k][1] = (i == 0 || j == 0 || k == 0) ? 100 : 255;
				fn_texture_data[i][j][k][2] = (i == 0 || j == 0 || k == 0) ? 100 : 255;
			}
		}
	}

	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB, fn_tex_size, fn_tex_size, fn_tex_size, 0, GL_RGB, GL_UNSIGNED_BYTE, fn_texture_data);

	glGenerateMipmap(GL_TEXTURE_3D);

	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	//glTexParameterf(GL_TEXTURE_3D, GL_TEXTURE_MAX_ANISOTROPY_EXT, Largest);

	glBindTexture(GL_TEXTURE_3D, 0);

	return (m_nFunctionTexture != 0);
}


//-----------------------------------------------------------------------------
// Purpose: Draw all of the controllers as X/Y/Z lines
//-----------------------------------------------------------------------------
void CMainApplication::RenderControllerAxes()
{
	// don't draw controllers if somebody else has input focus
	if( m_pHMD->IsInputFocusCapturedByAnotherProcess() )
		return;

	std::vector<float> vertdataarray;

	m_uiControllerVertcount = 0;
	m_iTrackedControllerCount = 0;

	for ( vr::TrackedDeviceIndex_t unTrackedDevice = vr::k_unTrackedDeviceIndex_Hmd + 1; unTrackedDevice < vr::k_unMaxTrackedDeviceCount; ++unTrackedDevice )
	{
		if ( !m_pHMD->IsTrackedDeviceConnected( unTrackedDevice ) )
			continue;

		if( m_pHMD->GetTrackedDeviceClass( unTrackedDevice ) != vr::TrackedDeviceClass_Controller )
			continue;

		m_iTrackedControllerCount += 1;

		if( !m_rTrackedDevicePose[ unTrackedDevice ].bPoseIsValid )
			continue;

		const Matrix4 & mat = m_rmat4DevicePose[unTrackedDevice];

		Vector4 center = mat * Vector4( 0, 0, 0, 1 );

		for ( int i = 0; i < 3; ++i )
		{
			Vector3 color( 0, 0, 0 );
			Vector4 point( 0, 0, 0, 1 );
			point[i] += 0.05f;  // offset in X, Y, Z
			color[i] = 1.0;  // R, G, B
			point = mat * point;
			vertdataarray.push_back( center.x );
			vertdataarray.push_back( center.y );
			vertdataarray.push_back( center.z );

			vertdataarray.push_back( color.x );
			vertdataarray.push_back( color.y );
			vertdataarray.push_back( color.z );
		
			vertdataarray.push_back( point.x );
			vertdataarray.push_back( point.y );
			vertdataarray.push_back( point.z );
		
			vertdataarray.push_back( color.x );
			vertdataarray.push_back( color.y );
			vertdataarray.push_back( color.z );
		
			m_uiControllerVertcount += 2;
		}

		Vector4 start = mat * Vector4( 0, 0, -0.02f, 1 );
		Vector4 end = mat * Vector4( 0, 0, -39.f, 1 );
		Vector3 color( .92f, .92f, .71f );

		vertdataarray.push_back( start.x );vertdataarray.push_back( start.y );vertdataarray.push_back( start.z );
		vertdataarray.push_back( color.x );vertdataarray.push_back( color.y );vertdataarray.push_back( color.z );

		vertdataarray.push_back( end.x );vertdataarray.push_back( end.y );vertdataarray.push_back( end.z );
		vertdataarray.push_back( color.x );vertdataarray.push_back( color.y );vertdataarray.push_back( color.z );
		m_uiControllerVertcount += 2;
	}

	// Setup the VAO the first time through.
	if ( m_unControllerVAO == 0 )
	{
		glGenVertexArrays( 1, &m_unControllerVAO );
		glBindVertexArray( m_unControllerVAO );

		glGenBuffers( 1, &m_glControllerVertBuffer );
		glBindBuffer( GL_ARRAY_BUFFER, m_glControllerVertBuffer );

		GLuint stride = 2 * 3 * sizeof( float );
		uintptr_t offset = 0;

		glEnableVertexAttribArray( 0 );
		glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, stride, (const void *)offset);

		offset += sizeof( Vector3 );
		glEnableVertexAttribArray( 1 );
		glVertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, stride, (const void *)offset);

		glBindVertexArray( 0 );
	}

	glBindBuffer( GL_ARRAY_BUFFER, m_glControllerVertBuffer );

	// set vertex data if we have some
	if( vertdataarray.size() > 0 )
	{
		//$ TODO: Use glBufferSubData for this...
		glBufferData( GL_ARRAY_BUFFER, sizeof(float) * vertdataarray.size(), &vertdataarray[0], GL_STREAM_DRAW );
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMainApplication::SetupCameras()
{
	m_mat4ProjectionLeft = GetHMDMatrixProjectionEye( vr::Eye_Left );
	m_mat4ProjectionRight = GetHMDMatrixProjectionEye( vr::Eye_Right );
	m_mat4eyePosLeft = GetHMDMatrixPoseEye( vr::Eye_Left );
	m_mat4eyePosRight = GetHMDMatrixPoseEye( vr::Eye_Right );
}

void CMainApplication::SetupFunction()
{
	std::stringstream sstream;

	srand(time(NULL));
	sstream << rand() % 10 - 5 << "x^3 + "
		<< rand() % 10 - 5 << "y^3 + "
		<< rand() % 10 - 5 << "z^3 + "
		<< rand() % 10 - 5 << "x^2y + "
		<< rand() % 10 - 5 << "xy^2 + "
		<< rand() % 10 - 5 << "x^2z + "
		<< rand() % 10 - 5 << "xz^2 + "
		<< rand() % 10 - 5 << "y^2z + "
		<< rand() % 10 - 5 << "yz^2 + "
		<< rand() % 10 - 5 << "x^2 + "
		<< rand() % 10 - 5 << "y^2 + "
		<< rand() % 10 - 5 << "z^2 + "
		<< rand() % 10 - 5 << "xy + "
		<< rand() % 10 - 5 << "xz + "
		<< rand() % 10 - 5 << "yz + "
		<< rand() % 10 - 5 << "x + "
		<< rand() % 10 - 5 << "y + "
		<< rand() % 10 - 5 << "z";

	Term* temp_term = Term::parseTerm(sstream.str());
	m_functionTextInput.set_str(sstream.str());

	int degree;
	m_function = temp_term->homogenize(&degree);

	delete temp_term;

	std::cout << "Function initialized. Building mesh..." << std::endl;

	m_functionMesh = new FunctionMesh(m_function);

	std::cout << "Mesh built." << std::endl;

	glGenBuffers(1, &m_functionVertBuffer);
	glGenBuffers(1, &m_functionNormalBuffer);

	glGenVertexArrays(1, &m_functionVAO);

	m_functionPose.translate(Vector3(0, 1, 0));

	// Debug Cubes stuff
	glGenBuffers(1, &m_debugCubesVertBuffer);
	glGenBuffers(1, &m_debugCubesColorBuffer);

	glGenVertexArrays(1, &m_debugCubesVAO);
}

void CMainApplication::AsynchReplaceFunction()
{
	m_FunctionMeshUnderConstruction = new FunctionMesh(m_functionUnderConstruction);

	m_bFunctionMeshIsUnderConstruction = false;
}

void CMainApplication::CreateAsynchReplaceFunctionThread()
{
	try
	{
		Term* temp_term = Term::parseTerm(m_functionTextInput.get_str());
		int degree;
		Term* hommed_term = temp_term->homogenize(&degree);

		delete(temp_term);

		// Got this far, so term is well-formed.
		m_bFunctionMeshIsUnderConstruction = true;
		m_functionUnderConstruction = hommed_term;

		DWORD threadID;
		HANDLE asynch_thread;

		asynch_thread = CreateThread(NULL, 0, AsynchReplaceFunctionStarter, this, 0, &threadID);

	}
	catch (BadTermException bte)
	{
		dprintf("%s\n", bte.getErrorMessage());
	}
}

void CMainApplication::RenderFunction( vr::Hmd_Eye nEye )
{
	int num_vertices = m_functionMesh->vertices.size();

	// Slight abbreviation.
	std::vector<Vector4>& mesh_vertices = m_functionMesh->vertices;
	std::vector<Vector4>& mesh_gradients = m_functionMesh->gradients;

	std::vector<Vector4> culled_rotated_vertices;
	std::vector<Vector3> normals_list;
	culled_rotated_vertices.reserve(num_vertices);
	normals_list.reserve(num_vertices);

	Matrix4 currentFunctionPose =
		m_bTriggerIsHeld ? (m_fromTriggerPressedPose * m_functionPose) : 
		(m_bRotatingThroughInfinity ? m_temporaryRotation * m_functionPose :
			m_functionPose);

	for (int i = 0; i < num_vertices / 3; i++)
	{
		Vector4 p1 = currentFunctionPose*mesh_vertices[3 * i];
		Vector4 p2 = currentFunctionPose*mesh_vertices[3 * i + 1];
		Vector4 p3 = currentFunctionPose*mesh_vertices[3 * i + 2];

		if ((p1.w > 0 && p2.w > 0 && p3.w > 0) || (p1.w < 0 && p2.w < 0 && p3.w < 0))
		{
			p1 /= p1.w;
			p2 /= p2.w;
			p3 /= p3.w;

			Vector4 n1 = currentFunctionPose*mesh_gradients[3 * i];
			Vector4 n2 = currentFunctionPose*mesh_gradients[3 * i + 1];
			Vector4 n3 = currentFunctionPose*mesh_gradients[3 * i + 2];

			n1.normalize();
			n2.normalize();
			n3.normalize();

			normals_list.push_back(Vector3(n1.x,n1.y,n1.z));
			normals_list.push_back(Vector3(n2.x,n2.y,n2.z));
			normals_list.push_back(Vector3(n3.x,n3.y,n3.z));

			culled_rotated_vertices.push_back(p1);
			culled_rotated_vertices.push_back(p2);
			culled_rotated_vertices.push_back(p3);
		}
	}

	int num_culled_vertices = culled_rotated_vertices.size();

	// GL stuff for surface.
	glUseProgram(m_unFunctionProgramID);

	glBindVertexArray(m_functionVAO);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	glUniformMatrix4fv(m_nFunctionMatrixLocation, 1, GL_FALSE, GetCurrentViewProjectionMatrix(nEye).get());

	if (m_bRotatingThroughInfinity)
	{
		glUniform3f(m_nFunctionSurfaceColorLocation, (float)0x32 / 0xff, (float)0x20 / 0xff, (float)0xaf / 0xff); // 3220afff
	}
	else if (m_bTriggerIsHeld) // Affine motion of surface
	{
		glUniform3f(m_nFunctionSurfaceColorLocation, (float)0x37 / 0xff, (float)0x9c / 0xff, (float)0xff / 0xff); // 379cffff // 30843eff
	}
	else // Nothing special happening
	{
		glUniform3f(m_nFunctionSurfaceColorLocation, (float)0x20 / 0xff, (float)0x6b / 0xff, (float)0xaf / 0xff); // 206bafff
	}

	glBindBuffer(GL_ARRAY_BUFFER, m_functionVertBuffer);
	glVertexAttribPointer(
		0,
		4,
		GL_FLOAT,
		GL_FALSE,
		0,
		(void*)0
	);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*num_culled_vertices * 4, &(culled_rotated_vertices[0]), GL_DYNAMIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, m_functionNormalBuffer);
	glVertexAttribPointer(
		1,
		3,
		GL_FLOAT,
		GL_FALSE,
		0,
		(void*)0
	);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*num_culled_vertices * 3, &(normals_list[0]), GL_DYNAMIC_DRAW);

	glBindTexture(GL_TEXTURE_3D, m_nFunctionTexture);

	glDrawArrays(GL_TRIANGLES, 0, num_culled_vertices);

	glBindVertexArray(0);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);

	glBindTexture(GL_TEXTURE_3D, 0);


	if (m_bDebugCubes)
	{
		// Culling stuff for debug cubes

		std::vector<Vector4> culled_debug_vertices;
		std::vector<Vector3> culled_debug_colors;
		culled_debug_vertices.reserve(m_functionMesh->debug_vertices.size());
		culled_debug_colors.reserve(m_functionMesh->debug_colors.size());

		for (int i = 0; i < m_functionMesh->debug_vertices.size() / 2; i++)
		{
			Vector4 p1 = currentFunctionPose*m_functionMesh->debug_vertices[2 * i];
			Vector4 p2 = currentFunctionPose*m_functionMesh->debug_vertices[2 * i + 1];

			if ((p1.w > 0 && p2.w > 0) || (p1.w < 0 && p2.w < 0))
			{
				p1 /= p1.w;
				p2 /= p2.w;

				culled_debug_vertices.push_back(p1);
				culled_debug_vertices.push_back(p2);

				culled_debug_colors.push_back(m_functionMesh->debug_colors[2 * i]);
				culled_debug_colors.push_back(m_functionMesh->debug_colors[2 * i + 1]);
			}
		}

		// GL stuff for debug cubes
		glUseProgram(m_unControllerTransformProgramID); // We can reuse this one.

		glBindVertexArray(m_debugCubesVAO);

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);

		glUniformMatrix4fv(m_nControllerMatrixLocation, 1, GL_FALSE, GetCurrentViewProjectionMatrix(nEye).get());

		glBindBuffer(GL_ARRAY_BUFFER, m_debugCubesVertBuffer);
		glVertexAttribPointer(
			0,
			4,
			GL_FLOAT,
			GL_FALSE,
			0,
			(void*)0
		);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float)*culled_debug_vertices.size() * 4, &(culled_debug_vertices[0]), GL_DYNAMIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, m_debugCubesColorBuffer);
		glVertexAttribPointer(
			1,
			3,
			GL_FLOAT,
			GL_FALSE,
			0,
			(void*)0
		);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float)*culled_debug_colors.size() * 3, &(culled_debug_colors[0]), GL_DYNAMIC_DRAW);

		glDrawArrays(GL_LINES, 0, culled_debug_vertices.size());

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
	}

}

void CMainApplication::RenderFunctionTextInput(vr::Hmd_Eye nEye)
{
	// Render text to a relevant texture

	glUseProgram(m_unCompanionWindowProgramID);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_functionTextInput.glFramebufferId);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE1, m_functionTextInput.glTextureId, 0);
	
	// They say not to use exceptions for control flow. Don't tell them.
	// It also might be better to do this somewhere other than the drawing code. Don't tell anyone.
	bool function_string_is_valid;
	try
	{
		Term* term = Term::parseTerm(m_functionTextInput.str);
		delete term;
		m_functionTextInput.is_correct = true;
		m_functionTextInput.ticks_when_last_valid = SDL_GetTicks();
	}
	catch (BadTermException bte)
	{
		function_string_is_valid = false;
		m_functionTextInput.is_correct = false;
	}

	Uint32 ticks_since_last_valid = SDL_GetTicks() - m_functionTextInput.ticks_when_last_valid;
	if (m_functionTextInput.is_correct || ticks_since_last_valid < 300)
		glClearColor(0, 0, 0, 1);
	else if (ticks_since_last_valid < 500)
	{
		float r = (float)(ticks_since_last_valid - 300) / 200;
		glClearColor(r * (float)170 / 256, r * (float)23 / 256, r * (float)2 / 256, 1);
	}
	else
		glClearColor((float)170/256, (float)23/256, (float)2/256, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	glClearColor(1, 1, 1, 1);

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glViewport(0, 0, m_functionTextInput.texture_width, m_functionTextInput.texture_height);

	glBindVertexArray(m_functionTextInput.glCharVAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_functionTextInput.glCharVertBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_functionTextInput.glCharIndexBuffer);

	float penx = -0.95;
	float peny = 0.85;
	float line_height = 0.08;

	FT_UInt glyph_index = FT_Get_Char_Index(m_robotoFace, 'A');
	FT_Load_Glyph(m_robotoFace, glyph_index, FT_LOAD_DEFAULT);
	float a_height = m_robotoFace->glyph->metrics.height;
	static bool printed = false;
	std::string str_to_show = m_functionTextInput.str;
	str_to_show.append(" = 0");
	for (int i = 0; i < str_to_show.length(); i++)
	{
		std::vector<VertexDataWindow> vVerts;

		char c = str_to_show[i];
		FT_UInt glyph_index = FT_Get_Char_Index(m_robotoFace, c);
		FT_Load_Glyph(m_robotoFace, glyph_index, FT_LOAD_DEFAULT);
		FT_Glyph_Metrics metrics = m_robotoFace->glyph->metrics;

		float bearingX = (line_height / a_height) * metrics.horiBearingX;
		float bearingY = (line_height / a_height) * metrics.horiBearingY;
		float width = (line_height / a_height) * metrics.width;
		float height = (line_height / a_height) * metrics.height;

		vVerts.push_back(VertexDataWindow(Vector2(penx + bearingX, peny + bearingY - height), Vector2(0, 1)));
		vVerts.push_back(VertexDataWindow(Vector2(penx + bearingX + width, peny + bearingY - height), Vector2(1, 1)));
		vVerts.push_back(VertexDataWindow(Vector2(penx + bearingX, peny + bearingY), Vector2(0, 0)));
		vVerts.push_back(VertexDataWindow(Vector2(penx + bearingX + width, peny + bearingY), Vector2(1, 0)));

		if (i + 1 < str_to_show.length())
		{
			// There's a next character, so compute kerning.
			FT_UInt next_glyph_index = FT_Get_Char_Index(m_robotoFace, str_to_show[i + 1]);
			FT_Vector delta;
			FT_Get_Kerning(m_robotoFace, glyph_index, next_glyph_index, FT_KERNING_DEFAULT, &delta);
			penx += (line_height / a_height) * delta.x;
		}

		penx += (line_height / a_height) * metrics.horiAdvance;

		glBufferData(GL_ARRAY_BUFFER, vVerts.size() * sizeof(VertexDataWindow), &vVerts[0], GL_DYNAMIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(VertexDataWindow), (void *)offsetof(VertexDataWindow, position));

		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VertexDataWindow), (void *)offsetof(VertexDataWindow, texCoord));

		glBindTexture(GL_TEXTURE_2D, m_functionTextInput.glCharTextures[c]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

		if (i + 1 == m_functionTextInput.cursor_pos)
		{
			// Gotta draw a cursor
			c = '|';
			glyph_index = FT_Get_Char_Index(m_robotoFace, c);
			FT_Load_Glyph(m_robotoFace, glyph_index, FT_LOAD_DEFAULT);
			metrics = m_robotoFace->glyph->metrics;

			bearingX = 0;
			bearingY = (line_height / a_height) * metrics.horiBearingY;
			width = (line_height / a_height) * metrics.width;
			height = (line_height / a_height) * metrics.height;

			vVerts.clear();
			vVerts.push_back(VertexDataWindow(Vector2(penx + bearingX, peny + bearingY - height), Vector2(0, 1)));
			vVerts.push_back(VertexDataWindow(Vector2(penx + bearingX + width, peny + bearingY - height), Vector2(1, 1)));
			vVerts.push_back(VertexDataWindow(Vector2(penx + bearingX, peny + bearingY), Vector2(0, 0)));
			vVerts.push_back(VertexDataWindow(Vector2(penx + bearingX + width, peny + bearingY), Vector2(1, 0)));

			glBufferData(GL_ARRAY_BUFFER, vVerts.size() * sizeof(VertexDataWindow), &vVerts[0], GL_DYNAMIC_DRAW);

			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(VertexDataWindow), (void *)offsetof(VertexDataWindow, position));

			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VertexDataWindow), (void *)offsetof(VertexDataWindow, texCoord));

			glBindTexture(GL_TEXTURE_2D, m_functionTextInput.glCharTextures[c]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
		}

		// rough line-wrapping
		if (penx > .9)
		{
			penx = -0.95;
			peny -= (line_height / a_height) * m_robotoFace->size->metrics.height;
		}


	}
	printed = true;


	glViewport(0, 0, m_nRenderWidth, m_nRenderHeight);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	if (nEye == vr::Eye_Left)
		glBindFramebuffer(GL_FRAMEBUFFER, leftEyeDesc.m_nRenderFramebufferId);
	else
		glBindFramebuffer(GL_FRAMEBUFFER, rightEyeDesc.m_nRenderFramebufferId);

    // Now draw the stuff in 3D
	glUseProgram(m_unSceneProgramID);

	glUniformMatrix4fv(m_nSceneMatrixLocation, 1, GL_FALSE, GetCurrentViewProjectionMatrix(nEye).get());

	glBindVertexArray(m_functionTextInput.glVAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_functionTextInput.glVertBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_functionTextInput.glIndexBuffer);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexDataScene), (void *)offsetof(VertexDataScene, position));

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VertexDataScene), (void *)offsetof(VertexDataScene, texCoord));

	glBindTexture(GL_TEXTURE_2D, m_functionTextInput.glTextureId);
	glDrawElements(GL_TRIANGLES, 6 /* Two triangles */, GL_UNSIGNED_SHORT, 0);

	glUseProgram(0);

}

// Does everything except set the text for the textinput.
void CMainApplication::SetupFunctionTextInput()
{
	FT_Init_FreeType(&m_ftLibrary);

	if (FT_New_Face(m_ftLibrary,
		"Roboto-Regular.ttf",
		0,
		&m_robotoFace))
	{
		dprintf("Trouble loading font.\n");
	}

	if (FT_Set_Pixel_Sizes(m_robotoFace, 0, m_functionTextInput.char_texture_height))
	{
		dprintf("Trouble setting size of font.\n");
	}

	// Set up textures for the characters we use.

	for (int i = 0; i < 256; i++)
	{
		FT_UInt glyph_index = FT_Get_Char_Index(m_robotoFace, i);
		if (glyph_index == 0)
		{
			dprintf("Glyph %i: %c not found.\n", i, i);
			continue;
		}
		if (FT_Load_Glyph(m_robotoFace, glyph_index, FT_LOAD_DEFAULT) != 0)
		{
			dprintf("Trouble loading glpyh %c.\n", i);
			continue;
		}
		//dprintf("About to render %ith texture...\n", i);

		if (FT_Render_Glyph(m_robotoFace->glyph, FT_RENDER_MODE_NORMAL))
		{
			dprintf("Trouble rendering glyph %i: %c\n", i, i);
			continue;
		}

		if (m_robotoFace->glyph->bitmap.buffer == 0)
		{
			dprintf("Empty bitmap for glyph %i\n", i);
			continue;
		}

		glGenTextures(1, &m_functionTextInput.glCharTextures[i]);
		glBindTexture(GL_TEXTURE_2D, m_functionTextInput.glCharTextures[i]);

		int width = m_robotoFace->glyph->bitmap.width;
		int height = m_robotoFace->glyph->bitmap.rows;

		dprintf("%c has a %ix%i size texture.\n", i, width, height);

		// To avoid what looks like a bug in my OpenGL implementation, I'm manually converting
		// from grayscale to RGBA, then passing this to OpenGL.
		std::vector<Vector4> buffer_vec;
		
		for (int j = 0; j < m_robotoFace->glyph->bitmap.rows; j++)
			for (int i = 0; i < m_robotoFace->glyph->bitmap.width; i++)
				buffer_vec.push_back(Vector4(
					1,
					1,
					1,
					(float)(*(m_robotoFace->glyph->bitmap.buffer + width*j + i)) / 256.00));
		
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height,
			0, GL_RGBA, GL_FLOAT, &buffer_vec[0]);
	}
	glBindTexture(GL_TEXTURE_2D, 0);

	// Then vertex, index arrays and VAO for individual characters.

	glGenBuffers(1, &m_functionTextInput.glCharVertBuffer);
	glGenBuffers(1, &m_functionTextInput.glCharIndexBuffer);

	glGenVertexArrays(1, &m_functionTextInput.glCharVAO);
	glBindVertexArray(m_functionTextInput.glCharVAO);

	glBindBuffer(GL_ARRAY_BUFFER, m_functionTextInput.glCharVertBuffer);

	GLushort vIndices[] = { 0, 1, 3,   0, 3, 2 };
	int num_indices = _countof(vIndices);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_functionTextInput.glCharIndexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, num_indices * sizeof(GLushort), &vIndices[0], GL_STATIC_DRAW);

	// Set up the 3D part
	std::vector<VertexDataScene> vVerts;

	float size = .25;
	float centerx = 0;
	float centery = 1;
	vVerts.push_back(VertexDataScene(Vector3(centerx - size, centery + size, 1), Vector2(1, 1)));
	vVerts.push_back(VertexDataScene(Vector3(centerx + size, centery + size, 1), Vector2(0, 1)));
	vVerts.push_back(VertexDataScene(Vector3(centerx - size, centery - size, 1), Vector2(1, 0)));
	vVerts.push_back(VertexDataScene(Vector3(centerx + size, centery - size, 1), Vector2(0, 0)));

	glGenVertexArrays(1, &m_functionTextInput.glVAO);
	glBindVertexArray(m_functionTextInput.glVAO);

	glGenBuffers(1, &m_functionTextInput.glVertBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, m_functionTextInput.glVertBuffer);
	glBufferData(GL_ARRAY_BUFFER, vVerts.size() * sizeof(VertexDataScene), &vVerts[0], GL_STATIC_DRAW);

	glGenBuffers(1, &m_functionTextInput.glIndexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_functionTextInput.glIndexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, num_indices * sizeof(GLushort), &vIndices[0], GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexDataScene), (void *)offsetof(VertexDataScene, position));

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VertexDataScene), (void *)offsetof(VertexDataScene, texCoord));

	glBindVertexArray(0);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// Set up the texture to render text to.

	glGenTextures(1, &m_functionTextInput.glTextureId);

	glBindTexture(GL_TEXTURE_2D, m_functionTextInput.glTextureId);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_functionTextInput.texture_width,
		m_functionTextInput.texture_height, 0, GL_RGB, GL_FLOAT, 0);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

	glBindTexture(GL_TEXTURE_2D, 0);

	glGenFramebuffers(1, &m_functionTextInput.glFramebufferId);
	
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_functionTextInput.glFramebufferId);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_functionTextInput.glTextureId, 0);

	glClearColor(0.8,0.8,0.8,1);
	glClear(GL_COLOR_BUFFER_BIT);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}



//-----------------------------------------------------------------------------
// Purpose: Creates a frame buffer. Returns true if the buffer was set up.
//          Returns false if the setup failed.
//-----------------------------------------------------------------------------
bool CMainApplication::CreateFrameBuffer( int nWidth, int nHeight, FramebufferDesc &framebufferDesc )
{
	glGenFramebuffers(1, &framebufferDesc.m_nRenderFramebufferId );
	glBindFramebuffer(GL_FRAMEBUFFER, framebufferDesc.m_nRenderFramebufferId);

	glGenRenderbuffers(1, &framebufferDesc.m_nDepthBufferId);
	glBindRenderbuffer(GL_RENDERBUFFER, framebufferDesc.m_nDepthBufferId);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT, nWidth, nHeight );
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,	framebufferDesc.m_nDepthBufferId );

	glGenTextures(1, &framebufferDesc.m_nRenderTextureId );
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, framebufferDesc.m_nRenderTextureId );
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA8, nWidth, nHeight, true);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, framebufferDesc.m_nRenderTextureId, 0);

	glGenFramebuffers(1, &framebufferDesc.m_nResolveFramebufferId );
	glBindFramebuffer(GL_FRAMEBUFFER, framebufferDesc.m_nResolveFramebufferId);

	glGenTextures(1, &framebufferDesc.m_nResolveTextureId );
	glBindTexture(GL_TEXTURE_2D, framebufferDesc.m_nResolveTextureId );
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, nWidth, nHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebufferDesc.m_nResolveTextureId, 0);

	// check FBO status
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		return false;
	}

	glBindFramebuffer( GL_FRAMEBUFFER, 0 );

	return true;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CMainApplication::SetupStereoRenderTargets()
{
	if ( !m_pHMD )
		return false;

	m_pHMD->GetRecommendedRenderTargetSize( &m_nRenderWidth, &m_nRenderHeight );

	CreateFrameBuffer( m_nRenderWidth, m_nRenderHeight, leftEyeDesc );
	CreateFrameBuffer( m_nRenderWidth, m_nRenderHeight, rightEyeDesc );
	
	return true;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMainApplication::SetupCompanionWindow()
{
	if ( !m_pHMD )
		return;

	std::vector<VertexDataWindow> vVerts;

	// left eye verts
	vVerts.push_back(VertexDataWindow(Vector2(-1, -1), Vector2(0, 0)));
	vVerts.push_back(VertexDataWindow(Vector2(0, -1), Vector2(1, 0)));
	vVerts.push_back( VertexDataWindow( Vector2(-1, 1), Vector2(0, 1)) );
	vVerts.push_back( VertexDataWindow( Vector2(0, 1), Vector2(1, 1)) );

	// right eye verts
	vVerts.push_back( VertexDataWindow( Vector2(0, -1), Vector2(0, 0)) );
	vVerts.push_back( VertexDataWindow( Vector2(1, -1), Vector2(1, 0)) );
	vVerts.push_back( VertexDataWindow( Vector2(0, 1), Vector2(0, 1)) );
	vVerts.push_back( VertexDataWindow( Vector2(1, 1), Vector2(1, 1)) );

	GLushort vIndices[] = { 0, 1, 3,   0, 3, 2,   4, 5, 7,   4, 7, 6};
	m_uiCompanionWindowIndexSize = _countof(vIndices);

	glGenVertexArrays( 1, &m_unCompanionWindowVAO );
	glBindVertexArray( m_unCompanionWindowVAO );

	glGenBuffers( 1, &m_glCompanionWindowIDVertBuffer );
	glBindBuffer( GL_ARRAY_BUFFER, m_glCompanionWindowIDVertBuffer );
	glBufferData( GL_ARRAY_BUFFER, vVerts.size()*sizeof(VertexDataWindow), &vVerts[0], GL_STATIC_DRAW );

	glGenBuffers( 1, &m_glCompanionWindowIDIndexBuffer );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_glCompanionWindowIDIndexBuffer );
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, m_uiCompanionWindowIndexSize*sizeof(GLushort), &vIndices[0], GL_STATIC_DRAW );

	glEnableVertexAttribArray( 0 );
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(VertexDataWindow), (void *)offsetof( VertexDataWindow, position ) );

	glEnableVertexAttribArray( 1 );
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VertexDataWindow), (void *)offsetof( VertexDataWindow, texCoord ) );

	glBindVertexArray( 0 );

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMainApplication::RenderStereoTargets()
{
	glClearColor( 1.0f, 1.0f, 1.0f, 1.0f );
	glEnable( GL_MULTISAMPLE );

	// Left Eye
	glBindFramebuffer( GL_FRAMEBUFFER, leftEyeDesc.m_nRenderFramebufferId );
 	glViewport(0, 0, m_nRenderWidth, m_nRenderHeight );
 	RenderScene( vr::Eye_Left );
 	glBindFramebuffer( GL_FRAMEBUFFER, 0 );
	
	glDisable( GL_MULTISAMPLE );
	 	
 	glBindFramebuffer(GL_READ_FRAMEBUFFER, leftEyeDesc.m_nRenderFramebufferId);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, leftEyeDesc.m_nResolveFramebufferId );

    glBlitFramebuffer( 0, 0, m_nRenderWidth, m_nRenderHeight, 0, 0, m_nRenderWidth, m_nRenderHeight, 
		GL_COLOR_BUFFER_BIT,
 		GL_LINEAR );

 	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0 );	

	glEnable( GL_MULTISAMPLE );

	// Right Eye
	glBindFramebuffer( GL_FRAMEBUFFER, rightEyeDesc.m_nRenderFramebufferId );
 	glViewport(0, 0, m_nRenderWidth, m_nRenderHeight );
 	RenderScene( vr::Eye_Right );
 	glBindFramebuffer( GL_FRAMEBUFFER, 0 );
 	
	glDisable( GL_MULTISAMPLE );

 	glBindFramebuffer(GL_READ_FRAMEBUFFER, rightEyeDesc.m_nRenderFramebufferId );
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, rightEyeDesc.m_nResolveFramebufferId );
	
    glBlitFramebuffer( 0, 0, m_nRenderWidth, m_nRenderHeight, 0, 0, m_nRenderWidth, m_nRenderHeight, 
		GL_COLOR_BUFFER_BIT,
 		GL_LINEAR  );

 	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0 );
}


//-----------------------------------------------------------------------------
// Purpose: Renders a scene with respect to nEye.
//-----------------------------------------------------------------------------
void CMainApplication::RenderScene( vr::Hmd_Eye nEye )
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	if (m_bInMenu)
		RenderFunctionTextInput(nEye);
	else
	    RenderFunction(nEye);

	bool bIsInputCapturedByAnotherProcess = m_pHMD->IsInputFocusCapturedByAnotherProcess();

	if( !bIsInputCapturedByAnotherProcess )
	{
		// draw the controller axis lines
		glUseProgram( m_unControllerTransformProgramID );
		glUniformMatrix4fv( m_nControllerMatrixLocation, 1, GL_FALSE, GetCurrentViewProjectionMatrix( nEye ).get() );
		glBindVertexArray( m_unControllerVAO );
		glDrawArrays( GL_LINES, 0, m_uiControllerVertcount );
		glBindVertexArray( 0 );
	}

	// ----- Render Model rendering -----
	glUseProgram( m_unRenderModelProgramID );

	for( uint32_t unTrackedDevice = 0; unTrackedDevice < vr::k_unMaxTrackedDeviceCount; unTrackedDevice++ )
	{
		if( !m_rTrackedDeviceToRenderModel[ unTrackedDevice ] || !m_rbShowTrackedDevice[ unTrackedDevice ] )
			continue;

		const vr::TrackedDevicePose_t & pose = m_rTrackedDevicePose[ unTrackedDevice ];
		if( !pose.bPoseIsValid )
			continue;

		if( bIsInputCapturedByAnotherProcess && m_pHMD->GetTrackedDeviceClass( unTrackedDevice ) == vr::TrackedDeviceClass_Controller )
			continue;

		if (m_pHMD->GetTrackedDeviceClass(unTrackedDevice) == vr::TrackedDeviceClass_HMD)
			continue;

		const Matrix4 & matDeviceToTracking = m_rmat4DevicePose[ unTrackedDevice ];
		Matrix4 matMVP = GetCurrentViewProjectionMatrix( nEye ) * matDeviceToTracking;
		glUniformMatrix4fv( m_nRenderModelMatrixLocation, 1, GL_FALSE, matMVP.get() );

		m_rTrackedDeviceToRenderModel[ unTrackedDevice ]->Draw();
	}

	glUseProgram( 0 );
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMainApplication::RenderCompanionWindow()
{
	glDisable(GL_DEPTH_TEST);
	glViewport( 0, 0, m_nCompanionWindowWidth, m_nCompanionWindowHeight );

	glBindVertexArray( m_unCompanionWindowVAO );
	glUseProgram( m_unCompanionWindowProgramID );

	// render left eye (first half of index array )
	glBindTexture(GL_TEXTURE_2D, leftEyeDesc.m_nResolveTextureId );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glDrawElements( GL_TRIANGLES, m_uiCompanionWindowIndexSize/2, GL_UNSIGNED_SHORT, 0 );

	// render right eye (second half of index array )
	glBindTexture(GL_TEXTURE_2D, rightEyeDesc.m_nResolveTextureId  );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glDrawElements( GL_TRIANGLES, m_uiCompanionWindowIndexSize/2, GL_UNSIGNED_SHORT, (const void *)(uintptr_t)(m_uiCompanionWindowIndexSize) );

	glBindVertexArray( 0 );
	glUseProgram( 0 );
}


//-----------------------------------------------------------------------------
// Purpose: Gets a Matrix Projection Eye with respect to nEye.
//-----------------------------------------------------------------------------
Matrix4 CMainApplication::GetHMDMatrixProjectionEye( vr::Hmd_Eye nEye )
{
	if ( !m_pHMD )
		return Matrix4();

	vr::HmdMatrix44_t mat = m_pHMD->GetProjectionMatrix( nEye, m_fNearClip, m_fFarClip );

	return Matrix4(
		mat.m[0][0], mat.m[1][0], mat.m[2][0], mat.m[3][0],
		mat.m[0][1], mat.m[1][1], mat.m[2][1], mat.m[3][1], 
		mat.m[0][2], mat.m[1][2], mat.m[2][2], mat.m[3][2], 
		mat.m[0][3], mat.m[1][3], mat.m[2][3], mat.m[3][3]
	);
}


//-----------------------------------------------------------------------------
// Purpose: Gets an HMDMatrixPoseEye with respect to nEye.
//-----------------------------------------------------------------------------
Matrix4 CMainApplication::GetHMDMatrixPoseEye( vr::Hmd_Eye nEye )
{
	if ( !m_pHMD )
		return Matrix4();

	vr::HmdMatrix34_t matEyeRight = m_pHMD->GetEyeToHeadTransform( nEye );
	Matrix4 matrixObj(
		matEyeRight.m[0][0], matEyeRight.m[1][0], matEyeRight.m[2][0], 0.0, 
		matEyeRight.m[0][1], matEyeRight.m[1][1], matEyeRight.m[2][1], 0.0,
		matEyeRight.m[0][2], matEyeRight.m[1][2], matEyeRight.m[2][2], 0.0,
		matEyeRight.m[0][3], matEyeRight.m[1][3], matEyeRight.m[2][3], 1.0f
		);

	return matrixObj.invert();
}


//-----------------------------------------------------------------------------
// Purpose: Gets a Current View Projection Matrix with respect to nEye,
//          which may be an Eye_Left or an Eye_Right.
//-----------------------------------------------------------------------------
Matrix4 CMainApplication::GetCurrentViewProjectionMatrix( vr::Hmd_Eye nEye )
{
	Matrix4 matMVP;
	if( nEye == vr::Eye_Left )
	{
		matMVP = m_mat4ProjectionLeft * m_mat4eyePosLeft * m_mat4HMDPose;
	}
	else if( nEye == vr::Eye_Right )
	{
		matMVP = m_mat4ProjectionRight * m_mat4eyePosRight *  m_mat4HMDPose;
	}

	return matMVP;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMainApplication::UpdateHMDMatrixPose()
{
	if ( !m_pHMD )
		return;

	vr::VRCompositor()->WaitGetPoses(m_rTrackedDevicePose, vr::k_unMaxTrackedDeviceCount, NULL, 0 );

	m_iValidPoseCount = 0;
	m_strPoseClasses = "";
	for ( int nDevice = 0; nDevice < vr::k_unMaxTrackedDeviceCount; ++nDevice )
	{
		if ( m_rTrackedDevicePose[nDevice].bPoseIsValid )
		{
			m_iValidPoseCount++;
			m_rmat4DevicePose[nDevice] = ConvertSteamVRMatrixToMatrix4( m_rTrackedDevicePose[nDevice].mDeviceToAbsoluteTracking );
			if (m_rDevClassChar[nDevice]==0)
			{
				switch (m_pHMD->GetTrackedDeviceClass(nDevice))
				{
				case vr::TrackedDeviceClass_Controller:        m_rDevClassChar[nDevice] = 'C'; break;
				case vr::TrackedDeviceClass_HMD:               m_rDevClassChar[nDevice] = 'H'; break;
				case vr::TrackedDeviceClass_Invalid:           m_rDevClassChar[nDevice] = 'I'; break;
				case vr::TrackedDeviceClass_GenericTracker:    m_rDevClassChar[nDevice] = 'G'; break;
				case vr::TrackedDeviceClass_TrackingReference: m_rDevClassChar[nDevice] = 'T'; break;
				default:                                       m_rDevClassChar[nDevice] = '?'; break;
				}
			}
			m_strPoseClasses += m_rDevClassChar[nDevice];
		}
	}

	if ( m_rTrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid )
	{
		m_mat4HMDPose = m_rmat4DevicePose[vr::k_unTrackedDeviceIndex_Hmd];
		m_mat4HMDPose.invert();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Finds a render model we've already loaded or loads a new one
//-----------------------------------------------------------------------------
CGLRenderModel *CMainApplication::FindOrLoadRenderModel( const char *pchRenderModelName )
{
	CGLRenderModel *pRenderModel = NULL;
	for( std::vector< CGLRenderModel * >::iterator i = m_vecRenderModels.begin(); i != m_vecRenderModels.end(); i++ )
	{
		if( !stricmp( (*i)->GetName().c_str(), pchRenderModelName ) )
		{
			pRenderModel = *i;
			break;
		}
	}

	// load the model if we didn't find one
	if( !pRenderModel )
	{
		vr::RenderModel_t *pModel;
		vr::EVRRenderModelError error;
		while ( 1 )
		{
			error = vr::VRRenderModels()->LoadRenderModel_Async( pchRenderModelName, &pModel );
			if ( error != vr::VRRenderModelError_Loading )
				break;

			ThreadSleep( 1 );
		}

		if ( error != vr::VRRenderModelError_None )
		{
			dprintf( "Unable to load render model %s - %s\n", pchRenderModelName, vr::VRRenderModels()->GetRenderModelErrorNameFromEnum( error ) );
			return NULL; // move on to the next tracked device
		}

		vr::RenderModel_TextureMap_t *pTexture;
		while ( 1 )
		{
			error = vr::VRRenderModels()->LoadTexture_Async( pModel->diffuseTextureId, &pTexture );
			if ( error != vr::VRRenderModelError_Loading )
				break;

			ThreadSleep( 1 );
		}

		if ( error != vr::VRRenderModelError_None )
		{
			dprintf( "Unable to load render texture id:%d for render model %s\n", pModel->diffuseTextureId, pchRenderModelName );
			vr::VRRenderModels()->FreeRenderModel( pModel );
			return NULL; // move on to the next tracked device
		}

		pRenderModel = new CGLRenderModel( pchRenderModelName );
		if ( !pRenderModel->BInit( *pModel, *pTexture ) )
		{
			dprintf( "Unable to create GL model from render model %s\n", pchRenderModelName );
			delete pRenderModel;
			pRenderModel = NULL;
		}
		else
		{
			m_vecRenderModels.push_back( pRenderModel );
		}
		vr::VRRenderModels()->FreeRenderModel( pModel );
		vr::VRRenderModels()->FreeTexture( pTexture );
	}
	return pRenderModel;
}


//-----------------------------------------------------------------------------
// Purpose: Create/destroy GL a Render Model for a single tracked device
//-----------------------------------------------------------------------------
void CMainApplication::SetupRenderModelForTrackedDevice( vr::TrackedDeviceIndex_t unTrackedDeviceIndex )
{
	if( unTrackedDeviceIndex >= vr::k_unMaxTrackedDeviceCount )
		return;

	// try to find a model we've already set up
	std::string sRenderModelName = GetTrackedDeviceString( m_pHMD, unTrackedDeviceIndex, vr::Prop_RenderModelName_String );
	CGLRenderModel *pRenderModel = FindOrLoadRenderModel( sRenderModelName.c_str() );
	if( !pRenderModel )
	{
		std::string sTrackingSystemName = GetTrackedDeviceString( m_pHMD, unTrackedDeviceIndex, vr::Prop_TrackingSystemName_String );
		dprintf( "Unable to load render model for tracked device %d (%s.%s)", unTrackedDeviceIndex, sTrackingSystemName.c_str(), sRenderModelName.c_str() );
	}
	else
	{
		m_rTrackedDeviceToRenderModel[ unTrackedDeviceIndex ] = pRenderModel;
		m_rbShowTrackedDevice[ unTrackedDeviceIndex ] = true;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Create/destroy GL Render Models
//-----------------------------------------------------------------------------
void CMainApplication::SetupRenderModels()
{
	memset( m_rTrackedDeviceToRenderModel, 0, sizeof( m_rTrackedDeviceToRenderModel ) );

	if( !m_pHMD )
		return;

	for( uint32_t unTrackedDevice = vr::k_unTrackedDeviceIndex_Hmd + 1; unTrackedDevice < vr::k_unMaxTrackedDeviceCount; unTrackedDevice++ )
	{
		if( !m_pHMD->IsTrackedDeviceConnected( unTrackedDevice ) )
			continue;

		SetupRenderModelForTrackedDevice( unTrackedDevice );


	}

}


//-----------------------------------------------------------------------------
// Purpose: Converts a SteamVR matrix to our local matrix class
//-----------------------------------------------------------------------------
Matrix4 CMainApplication::ConvertSteamVRMatrixToMatrix4( const vr::HmdMatrix34_t &matPose )
{
	Matrix4 matrixObj(
		matPose.m[0][0], matPose.m[1][0], matPose.m[2][0], 0.0,
		matPose.m[0][1], matPose.m[1][1], matPose.m[2][1], 0.0,
		matPose.m[0][2], matPose.m[1][2], matPose.m[2][2], 0.0,
		matPose.m[0][3], matPose.m[1][3], matPose.m[2][3], 1.0f
		);
	return matrixObj;
}


//-----------------------------------------------------------------------------
// Purpose: Create/destroy GL Render Models
//-----------------------------------------------------------------------------
CGLRenderModel::CGLRenderModel( const std::string & sRenderModelName )
	: m_sModelName( sRenderModelName )
{
	m_glIndexBuffer = 0;
	m_glVertArray = 0;
	m_glVertBuffer = 0;
	m_glTexture = 0;
}


CGLRenderModel::~CGLRenderModel()
{
	Cleanup();
}


//-----------------------------------------------------------------------------
// Purpose: Allocates and populates the GL resources for a render model
//-----------------------------------------------------------------------------
bool CGLRenderModel::BInit( const vr::RenderModel_t & vrModel, const vr::RenderModel_TextureMap_t & vrDiffuseTexture )
{
	// create and bind a VAO to hold state for this model
	glGenVertexArrays( 1, &m_glVertArray );
	glBindVertexArray( m_glVertArray );

	// Populate a vertex buffer
	glGenBuffers( 1, &m_glVertBuffer );
	glBindBuffer( GL_ARRAY_BUFFER, m_glVertBuffer );
	glBufferData( GL_ARRAY_BUFFER, sizeof( vr::RenderModel_Vertex_t ) * vrModel.unVertexCount, vrModel.rVertexData, GL_STATIC_DRAW );

	// Identify the components in the vertex buffer
	glEnableVertexAttribArray( 0 );
	glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, sizeof( vr::RenderModel_Vertex_t ), (void *)offsetof( vr::RenderModel_Vertex_t, vPosition ) );
	glEnableVertexAttribArray( 1 );
	glVertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, sizeof( vr::RenderModel_Vertex_t ), (void *)offsetof( vr::RenderModel_Vertex_t, vNormal ) );
	glEnableVertexAttribArray( 2 );
	glVertexAttribPointer( 2, 2, GL_FLOAT, GL_FALSE, sizeof( vr::RenderModel_Vertex_t ), (void *)offsetof( vr::RenderModel_Vertex_t, rfTextureCoord ) );

	// Create and populate the index buffer
	glGenBuffers( 1, &m_glIndexBuffer );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_glIndexBuffer );
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( uint16_t ) * vrModel.unTriangleCount * 3, vrModel.rIndexData, GL_STATIC_DRAW );

	glBindVertexArray( 0 );

	// create and populate the texture
	glGenTextures(1, &m_glTexture );
	glBindTexture( GL_TEXTURE_2D, m_glTexture );

	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, vrDiffuseTexture.unWidth, vrDiffuseTexture.unHeight,
		0, GL_RGBA, GL_UNSIGNED_BYTE, vrDiffuseTexture.rubTextureMapData );

	// If this renders black ask McJohn what's wrong.
	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );

	GLfloat fLargest;
	glGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, fLargest );

	glBindTexture( GL_TEXTURE_2D, 0 );

	m_unVertexCount = vrModel.unTriangleCount * 3;

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Frees the GL resources for a render model
//-----------------------------------------------------------------------------
void CGLRenderModel::Cleanup()
{
	if( m_glVertBuffer )
	{
		glDeleteBuffers(1, &m_glIndexBuffer);
		glDeleteVertexArrays( 1, &m_glVertArray );
		glDeleteBuffers(1, &m_glVertBuffer);
		m_glIndexBuffer = 0;
		m_glVertArray = 0;
		m_glVertBuffer = 0;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Draws the render model
//-----------------------------------------------------------------------------
void CGLRenderModel::Draw()
{
	glBindVertexArray( m_glVertArray );

	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, m_glTexture );

	glDrawElements( GL_TRIANGLES, m_unVertexCount, GL_UNSIGNED_SHORT, 0 );

	glBindVertexArray( 0 );
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int main(int argc, char *argv[])
{
	CMainApplication *pMainApplication = new CMainApplication( argc, argv );

	if (!pMainApplication->BInit())
	{
		pMainApplication->Shutdown();
		return 1;
	}

	pMainApplication->RunMainLoop();

	pMainApplication->Shutdown();

	return 0;
}