#include "DebugCallback.h"

#include <QDebug>

// TODO:
//  This could be handy if we want to disable certain severity levels
//
// glDebugMessageControlARB(GL_DEBUG_SOURCE_API, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, GL_FALSE);
//

static const char* sourceToString(GLenum source)
{
    static_assert((GL_DEBUG_SOURCE_OTHER - GL_DEBUG_SOURCE_API) == 5, "GL_DEBUG_SOURCE constants are not contiguous.");

    static const char* GL_SourceStrings[] =
        {
            "API",            // GL_DEBUG_SOURCE_API
            "System",         // GL_DEBUG_SOURCE_WINDOW_SYSTEM
            "ShaderCompiler", // GL_DEBUG_SOURCE_SHADER_COMPILER
            "ThirdParty",     // GL_DEBUG_SOURCE_THIRD_PARTY
            "Application",    // GL_DEBUG_SOURCE_APPLICATION
            "Other"           // GL_DEBUG_SOURCE_OTHER
        };

    if ((source >= GL_DEBUG_SOURCE_API) && (source <= GL_DEBUG_SOURCE_OTHER))
        return GL_SourceStrings[source - GL_DEBUG_SOURCE_API];

    return "Unknown";
}



static const char* typeToString(GLenum Type)
{
    static_assert((GL_DEBUG_TYPE_OTHER - GL_DEBUG_TYPE_ERROR) == 5, "GL_DEBUG_TYPE constants are not contiguous.");
    static const char* TypeStrings[] =
        {
            "Error",             // GL_DEBUG_TYPE_ERROR
            "Deprecated",        // GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR
            "UndefinedBehavior", // GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR
            "Portability",       // GL_DEBUG_TYPE_PORTABILITY
            "Performance",       // GL_DEBUG_TYPE_PERFORMANCE
            "Other"              // GL_DEBUG_TYPE_OTHER
        };

    if ((Type >= GL_DEBUG_TYPE_ERROR) && (Type <= GL_DEBUG_TYPE_OTHER))
        return TypeStrings[Type - GL_DEBUG_TYPE_ERROR];

    // KHR_debug marker/push/pop functionality.
    static_assert((GL_DEBUG_TYPE_POP_GROUP - GL_DEBUG_TYPE_MARKER) == 2, "GL_DEBUG_TYPE constants are not contiguous.");
    static const char* TypeStrings2[] =
        {
            "Marker",    // GL_DEBUG_TYPE_MARKER
            "PushGroup", // GL_DEBUG_TYPE_PUSH_GROUP
            "PopGroup",  // GL_DEBUG_TYPE_POP_GROUP
        };

    if ((Type >= GL_DEBUG_TYPE_MARKER) && (Type <= GL_DEBUG_TYPE_POP_GROUP))
        return TypeStrings2[Type - GL_DEBUG_TYPE_MARKER];

    return "Unknown";
}

static const char* categoryToString(GLenum Category)
{
    static_assert((GL_DEBUG_CATEGORY_OTHER_AMD - GL_DEBUG_CATEGORY_API_ERROR_AMD) == 7, "GL_DEBUG_CATEGORY constants are not contiguous.");
    static const char* CategoryStrings[] =
        {
            "API",               // GL_DEBUG_CATEGORY_API_ERROR_AMD
            "System",            // GL_DEBUG_CATEGORY_WINDOW_SYSTEM_AMD
            "Deprecation",       // GL_DEBUG_CATEGORY_DEPRECATION_AMD
            "UndefinedBehavior", // GL_DEBUG_CATEGORY_UNDEFINED_BEHAVIOR_AMD
            "Performance",       // GL_DEBUG_CATEGORY_PERFORMANCE_AMD
            "ShaderCompiler",    // GL_DEBUG_CATEGORY_SHADER_COMPILER_AMD
            "Application",       // GL_DEBUG_CATEGORY_APPLICATION_AMD
            "Other"              // GL_DEBUG_CATEGORY_OTHER_AMD
        };

    if((Category >= GL_DEBUG_CATEGORY_API_ERROR_AMD) && (Category <= GL_DEBUG_CATEGORY_OTHER_AMD))
        return CategoryStrings[Category - GL_DEBUG_CATEGORY_API_ERROR_AMD];

    return "Unknown";
}

static void APIENTRY debugMessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei /*length*/, const GLchar* message, const void* /*userParam*/)
{
    const char* pSource   = sourceToString(source);
    const char* pType     = typeToString(type);

    switch(severity)
    {
    default:
    case GL_DEBUG_SEVERITY_NOTIFICATION:
        qInfo("[QuickVR] [GL Debug] [Notification] %s %s %#x: %s", pSource, pType, id, message);
        break;
    case GL_DEBUG_SEVERITY_LOW:
        qWarning("[QuickVR] [GL Debug] [Low] %s %s %#x: %s", pSource, pType, id, message);
        break;
    case GL_DEBUG_SEVERITY_MEDIUM:
        qWarning("[QuickVR] [GL Debug] [Medium] %s %s %#x: %s", pSource, pType, id, message);
        break;
    case GL_DEBUG_SEVERITY_HIGH:
        qCritical("[QuickVR] [GL Debug] [High] %s %s %#x: %s", pSource, pType, id,  message);
        break;
    }
}

static void APIENTRY debugMessageCallbackAMD(GLuint id, GLenum category, GLenum severity, GLsizei /*length*/, const GLchar* message, GLvoid* /*userParam*/)
{
    static_assert(GL_DEBUG_SEVERITY_LOW_AMD == GL_DEBUG_SEVERITY_LOW, "Severity mismatch"); // Verify that AMD_debug_output severity constants are identical to KHR_debug severity contstants.

    const char* pCategory = categoryToString(category);

    switch(severity)
    {
    default:
    case GL_DEBUG_SEVERITY_NOTIFICATION:
        qInfo("[QuickVR] [GL Debug] [Notification] %s %#x: %s", pCategory, id, message);
        break;
    case GL_DEBUG_SEVERITY_LOW:
        qWarning("[QuickVR] [GL Debug] [Low] %s %#x: %s", pCategory, id, message);
        break;
    case GL_DEBUG_SEVERITY_MEDIUM:
        qWarning("[QuickVR] [GL Debug] [Medium] %s %#x: %s", pCategory, id, message);
        break;
    case GL_DEBUG_SEVERITY_HIGH:
        qCritical("[QuickVR] [GL Debug] [High] %s %#x: %s", pCategory, id, message);
        break;
    }
}


void GL::DebugCallback::install(QOpenGLExtraFunctions *api)
{
    int err;
    while ((err = api->glGetError()) != GL_NO_ERROR);

#ifdef GL_KHR_debug
    api->glDebugMessageCallback(debugMessageCallback, nullptr);
    err = api->glGetError();
    if(err)
    {
        qCritical("[QuickVR] glDebugMessageCallback error: %x (%d)\n", err, err);
    }
    else
    {
        api->glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        err = api->glGetError();
        if(err)
        {
            qCritical("[QuickVR] GL_DEBUG_OUTPUT_SYNCHRONOUS error: %x (%d)\n", err, err);
        }
        else
        {
            return; // success!
        }
    }

#endif

#ifdef GL_ARB_debug_output
    PFNGLDEBUGMESSAGECALLBACKARBPROC glDebugMessageCallbackARB = (PFNGLDEBUGMESSAGECALLBACKARBPROC) QOpenGLContext::currentContext()->getProcAddress("glDebugMessageCallbackARB");
    if (glDebugMessageCallbackARB)
    {
        glDebugMessageCallbackARB(debugMessageCallback, nullptr);
        err = api->glGetError();
        if(err)
        {
            qCritical("[QuickVR] glDebugMessageCallbackARB error: %x (%d)\n", err, err);
        }
        else
        {
            api->glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
            err = api->glGetError();
            if(err)
            {
                qCritical("[QuickVR] GL_DEBUG_OUTPUT_SYNCHRONOUS error: %x (%d)\n", err, err);
            }
            else
            {
                return; // success!
            }
        }
    }
#endif

#ifdef GL_AMD_debug_output
    PFNGLDEBUGMESSAGECALLBACKAMDPROC glDebugMessageCallbackAMD = (PFNGLDEBUGMESSAGECALLBACKAMDPROC) QOpenGLContext::currentContext()->getProcAddress("glDebugMessageCallbackAMD");
    if (glDebugMessageCallbackAMD)
    {
        glDebugMessageCallbackAMD(debugMessageCallbackAMD, nullptr);
        err = api->glGetError();
        if(err)
        {
            qCritical("[QuickVR] glDebugMessageCallbackAMD error: %x (%d)\n", err, err);
        }
        else
        {
            return; // success!
        }
    }
#endif

    qFatal("Could not initialize DebugCallback!");
}





