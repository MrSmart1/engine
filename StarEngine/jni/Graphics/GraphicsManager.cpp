#include "GraphicsManager.h"
#include "../Logger.h"
#include "SpriteBatch.h"
#include "../Scenes/SceneManager.h"
#include "../Objects/BaseCamera.h"
#include "../Components/CameraComponent.h"
#include "../defines.h"

#ifdef DESKTOP

#include <wglext.h>
#else
#endif

#ifdef MOBILE
#include <GLES/gl.h>
#endif



namespace star
{
	GraphicsManager * GraphicsManager::mGraphicsManager = nullptr;
	
	GraphicsManager::~GraphicsManager()
	{
		star::Logger::GetInstance()->Log(star::LogLevel::Info, _T("Graphics Manager : Destructor"));
	}

	GraphicsManager::GraphicsManager() :
			mViewProjectionMatrix(),
			mViewInverseMatrix(),
			mProjectionMatrix(),
			mScreenResolution(0,0),
			mViewportResolution(0,0),
			mbHasWindowChanged(false),
			mIsInitialized(false),
			mWglSwapIntervalEXT(NULL),
			mWglGetSwapIntervalEXT(NULL)
	{
		star::Logger::GetInstance()->Log(star::LogLevel::Info, _T("Graphics Manager : Constructor"));
	}

	void GraphicsManager::CalculateViewPort()
	{
		//Calculate the correct viewport
		vec2 screenRes = GetWindowResolution();

		float width(0), height(0);
		int xOffset(0), yOffset(0);
		float aspectRatio(0);

		if(screenRes.x > screenRes.y)
		{
			height = screenRes.y;
			aspectRatio = (screenRes.x / screenRes.y);
			width = height * aspectRatio;

			xOffset = static_cast<int>((screenRes.x - width)/2);
		}
		else
		{
			width = screenRes.x;
			aspectRatio = (screenRes.y / screenRes.x);
			height = width * aspectRatio;

			yOffset = static_cast<int>((screenRes.y - height)/2);
		}

		glViewport(xOffset, yOffset, static_cast<int>(width), static_cast<int>(height));

		mViewportResolution.x = width;
		mViewportResolution.y = height;
	}

	GraphicsManager * GraphicsManager::GetInstance()
	{
		if(mGraphicsManager == nullptr)
		{
			mGraphicsManager = new GraphicsManager();
		}			
		return mGraphicsManager;	
	}

	void GraphicsManager::SetVSync(bool vSync)
	{
#ifdef DESKTOP
		//Enables or disables VSync.
		//0 = No Sync , 1+ = VSync
		//Default value is 1.
		if(!vSync)
		{
			mWglSwapIntervalEXT(0);
		}
		else
		{
			mWglSwapIntervalEXT(1);
		}
	#else
		Logger::GetInstance()->Log(LogLevel::Warning, 
			_T("Setting VSync on mobile is not supported. Default VSync is enabled"));
#endif
	}

	bool GraphicsManager::GetVSync() const
	{
#ifdef DESKTOP
		return !(mWglGetSwapIntervalEXT() == 0);
#else
		Logger::GetInstance()->Log(LogLevel::Warning, 
			_T("Toggeling VSync on mobile is not supported. Default VSync is enabled"));
		return true;
#endif
	}

#ifdef DESKTOP
	void GraphicsManager::Initialize(int32 screenWidth, int32 screenHeight)
	{
		if(!mIsInitialized)
		{
			mScreenResolution.x = float(screenWidth);
			mScreenResolution.y = float(screenHeight);
			glewInit();

			star::Logger::GetInstance()->Log(star::LogLevel::Info, _T("Graphics Manager : Initializing OpenGL Functors"));
			if(!InitializeOpenGLFunctors())
			{
				star::Logger::GetInstance()->Log(star::LogLevel::Error, _T("Graphics Manager : Graphics card doesn't support VSync option!!"));
			}

			SetVSync(true);

			//Initializes base GL state.
			// In a simple 2D game, we have control over the third
			// dimension. So we do not really need a Z-buffer.
			glDisable(GL_DEPTH_TEST);
			//[COMMENT] is this necessairy?
			glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
			mIsInitialized = true;
		}
	}
#else
	void GraphicsManager::Initialize(const android_app* pApplication)
	{
		if(!mIsInitialized)
		{
			star::Logger::GetInstance()->Log(star::LogLevel::Info, _T("Graphics Manager : Initialize"));
			EGLint lFormat, lNumConfigs, lErrorResult;
			EGLConfig lConfig;
			// Defines display requirements. 16bits mode here.
			const EGLint lAttributes[] = {
						EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
						EGL_BLUE_SIZE, 5, EGL_GREEN_SIZE, 6, EGL_RED_SIZE, 5,
						EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
						EGL_NONE
			};
			mDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
			if (mDisplay == EGL_NO_DISPLAY)
			{
				star::Logger::GetInstance()->Log(star::LogLevel::Error, _T("Graphics Manager : No display found"));
				return;
			}
			if (!eglInitialize(mDisplay, NULL, NULL))
			{
				star::Logger::GetInstance()->Log(star::LogLevel::Error, _T("Graphics Manager : Could not initialize display"));
				return;
			}
			if(!eglChooseConfig(mDisplay, lAttributes, &lConfig, 1,&lNumConfigs) || (lNumConfigs <= 0))
			{
				star::Logger::GetInstance()->Log(star::LogLevel::Error, _T("Graphics Manager : No display config"));
				return;
			}
			if (!eglGetConfigAttrib(mDisplay, lConfig,EGL_NATIVE_VISUAL_ID, &lFormat))
			{
				star::Logger::GetInstance()->Log(star::LogLevel::Error, _T("Graphics Manager : No config attributes"));
				return;
			}
			ANativeWindow_setBuffersGeometry(pApplication->window, 0, 0,lFormat);

			mSurface = eglCreateWindowSurface(mDisplay, lConfig, pApplication->window, NULL );
			if (mSurface == EGL_NO_SURFACE)
			{
				star::Logger::GetInstance()->Log(star::LogLevel::Error, _T("Graphics Manager : Could not create surface"));
				return;
			}
			EGLint contextAttrs[] = {
				 EGL_CONTEXT_CLIENT_VERSION, 2,
				 EGL_NONE
			};
			mContext = eglCreateContext(mDisplay, lConfig, EGL_NO_CONTEXT, contextAttrs);
			if (mContext == EGL_NO_CONTEXT)
			{
				star::Logger::GetInstance()->Log(star::LogLevel::Error, _T("Graphics Manager : Could not create context"));
				return;
			}
			EGLint sX(0), sY(0);
			if (!eglMakeCurrent(mDisplay, mSurface, mSurface, mContext)
			 || !eglQuerySurface(mDisplay, mSurface, EGL_WIDTH, &sX)
			 || !eglQuerySurface(mDisplay, mSurface, EGL_HEIGHT, &sY)
			 || (sX <= 0) || (sY <= 0))
			{
				star::Logger::GetInstance()->Log(star::LogLevel::Error, _T("Graphics Manager : Could not activate display"));
				return;
			}
			mViewportResolution.x = sX;
			mViewportResolution.y = sY;
			mScreenResolution = mViewportResolution;
			glViewport(0, 0, mViewportResolution.x, mViewportResolution.y);

			star::Logger::GetInstance()->Log(star::LogLevel::Info, _T("Graphics Manager : Initialized"));

			mIsInitialized = true;
		}
	}

	void GraphicsManager::Destroy()
	{
		star::Logger::GetInstance()->Log(star::LogLevel::Info, _T("Graphics Manager : Destroy"));
        // Destroys OpenGL context.
        if (mDisplay != EGL_NO_DISPLAY)
        {
            eglMakeCurrent(mDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE,EGL_NO_CONTEXT);
            if (mContext != EGL_NO_CONTEXT)
            {
                eglDestroyContext(mDisplay, mContext);
                mContext = EGL_NO_CONTEXT;
            }
            if (mSurface != EGL_NO_SURFACE)
            {
                eglDestroySurface(mDisplay, mSurface);
                mSurface = EGL_NO_SURFACE;	
            }
            eglTerminate(mDisplay);
            mDisplay = EGL_NO_DISPLAY;
            star::Logger::GetInstance()->Log(star::LogLevel::Info, _T("Graphics Manager : Destroyed"));

            SpriteBatch::GetInstance()->CleanUp();

            mIsInitialized = false;
        }
	}
#endif

	void GraphicsManager::StartDraw()
	{
		//star::Logger::GetInstance()->Log(star::LogLevel::Info, _T("Graphics Manager : StartDraw"));
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		/*
		GLint temp[4];
		glGetIntegerv(GL_VIEWPORT, temp);

		tstringstream buffer;
		buffer << _T("Viewport Width: ") << temp[2] << _T(" Viewport Height: ") << temp[3];
		Logger::GetInstance()->Log(LogLevel::Info, buffer.str());*/
	}

	void GraphicsManager::StopDraw()
	{
		//star::Logger::GetInstance()->Log(star::LogLevel::Info, _T("Graphics Manager : StopDraw"));
		 glDisable(GL_BLEND);
#ifdef ANDROID
		 if (eglSwapBuffers(mDisplay, mSurface) != EGL_TRUE)
		 {
			 return;
		 }
#endif
	}

	void GraphicsManager::Update()
	{
		if(SceneManager::GetInstance()->GetActiveScene())
		{
			auto projectionObject(SceneManager::GetInstance()->GetActiveScene()->GetActiveCamera());
			if(projectionObject)
			{
				const mat4x4& projection = projectionObject->GetComponent<CameraComponent>()
					->GetProjection();
				const mat4x4& viewInverse = projectionObject->GetComponent<CameraComponent>()
					->GetViewInverse();
				mProjectionMatrix = projection;
				mViewInverseMatrix = viewInverse;
				mViewProjectionMatrix = projection * viewInverse;
			}		
		}
	}

	int32 GraphicsManager::GetWindowWidth() const
	{
		return int32(mScreenResolution.x);
	}

	int32 GraphicsManager::GetWindowHeight() const
	{
		return int32(mScreenResolution.y);
	}

	const mat4x4& GraphicsManager::GetViewProjectionMatrix() const
	{
		return mViewProjectionMatrix;
	}

	const mat4x4& GraphicsManager::GetProjectionMatrix() const
	{
		return mProjectionMatrix;
	}

	const mat4x4& GraphicsManager::GetViewInverseMatrix() const
	{
		return mViewInverseMatrix;
	}

	float GraphicsManager::GetWindowAspectRatio() const
	{
		return mScreenResolution.x / mScreenResolution.y;
	}

	const vec2 & GraphicsManager::GetWindowResolution() const
	{
		return mScreenResolution;
	}

	const vec2 & GraphicsManager::GetViewportResolution() const
	{
		return mViewportResolution;
	}

	void GraphicsManager::SetWindowDimensions(int32 width, int32 height)
	{
		mScreenResolution.x = float(width);
		mScreenResolution.y = float(height);
		CalculateViewPort();
	}

	void GraphicsManager::SetHasWindowChanged(bool isTrue)
	{
		mbHasWindowChanged = isTrue;
		if(isTrue)
		{
			CalculateViewPort();
		}
	}

	bool GraphicsManager::GetHasWindowChanged() const
	{
		return mbHasWindowChanged;
	}

#ifdef DESKTOP

	bool GraphicsManager::WGLExtensionSupported(const char* extension_name)
	{
		// this is the pointer to the function which returns the pointer to string with the list of all wgl extensions
		PFNWGLGETEXTENSIONSSTRINGEXTPROC _wglGetExtensionsStringEXT = NULL;

		// determine pointer to wglGetExtensionsStringEXT function
		_wglGetExtensionsStringEXT = (PFNWGLGETEXTENSIONSSTRINGEXTPROC) wglGetProcAddress("wglGetExtensionsStringEXT");

		if (strstr(_wglGetExtensionsStringEXT(), extension_name) == NULL)
		{
			// string was not found
			return false;
		}

		// extension is supported
		return true;
	}

	bool GraphicsManager::InitializeOpenGLFunctors()
	{
		if (WGLExtensionSupported("WGL_EXT_swap_control"))
		{
			// Extension is supported, init pointers.
			mWglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC) wglGetProcAddress("wglSwapIntervalEXT");

			// this is another function from WGL_EXT_swap_control extension
			mWglGetSwapIntervalEXT = (PFNWGLGETSWAPINTERVALEXTPROC) wglGetProcAddress("wglGetSwapIntervalEXT");
			return true;
		}
		return false;
	}
#endif
}
