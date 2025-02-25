#include "glassert.h"

#include "api/gl_api_provider.hpp"

#ifndef NDEBUG
void render::gl::checkGlError(const char* code)
{
    const auto error = ::gl::getError();
    if(error == ::gl::ErrorCode::NoError)
        return;

    const char* errStr;
    switch(error)
    {
    case ::gl::ErrorCode::InvalidEnum: errStr = "invalid enum"; break;
    case ::gl::ErrorCode::InvalidOperation: errStr = "invalid operation"; break;
    case ::gl::ErrorCode::InvalidValue: errStr = "invalid value"; break;
    case ::gl::ErrorCode::OutOfMemory: errStr = "out of memory"; break;
    case ::gl::ErrorCode::StackOverflow: errStr = "stack overflow"; break;
    case ::gl::ErrorCode::StackUnderflow: errStr = "stack underflow"; break;
    case ::gl::ErrorCode::InvalidFramebufferOperation: errStr = "invalid framebuffer operation"; break;
    default: errStr = "<unknown error>";
    }

    BOOST_LOG_TRIVIAL(error) << "OpenGL error " << (::gl::core::EnumType)error << " after evaluation of '" << code
                             << "': " << errStr;
    BOOST_LOG_TRIVIAL(error) << "Stacktrace:\n" << boost::stacktrace::stacktrace();
    BOOST_ASSERT_MSG(false, code);
}
#endif
