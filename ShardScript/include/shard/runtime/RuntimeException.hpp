#pragma once

#include <shard/ShardScriptAPI.hpp>

#include <shard/semantic/symbols/TypeSymbol.hpp>

#include <exception>
#include <string>

namespace shard
{
    /// <summary>
    /// Native C++ exception type that the ShardScript VM maps to a managed IThrowable
    /// implementation. Native libraries can throw this exception to surface typed errors
    /// without manually allocating managed instances.
    /// </summary>
    class SHARD_API runtime_exception : public std::exception
    {
    public:
        /// <summary>
        /// Constructs a runtime exception with an optional managed type.
        /// </summary>
        /// <param name="message">Wide-string message exposed through IThrowable.message.</param>
        /// <param name="exceptionType">
        /// Managed exception class that implements IThrowable. If nullptr, the VM falls back
        /// to the built-in RuntimeException type.
        /// </param>
        runtime_exception(std::wstring message, TypeSymbol* exceptionType = nullptr) noexcept
            : m_message(std::move(message)), m_exceptionType(exceptionType) { }

        /// <summary>
        /// Constructs a runtime exception from a wide string literal.
        /// </summary>
        runtime_exception(const wchar_t* message, TypeSymbol* exceptionType = nullptr) noexcept
            : m_message(message != nullptr ? message : L""), m_exceptionType(exceptionType) { }

        /// <summary>
        /// Wide-string message carried by this exception.
        /// </summary>
        const std::wstring& message() const noexcept
        {
            return m_message;
        }

        /// <summary>
        /// Native stack trace captured at the point the exception is mapped to a managed object.
        /// </summary>
        const std::wstring& stack_trace() const noexcept
        {
            return m_stackTrace;
        }

        /// <summary>
        /// Sets the stack trace. Called by the VM before the managed instance is created.
        /// </summary>
        void set_stack_trace(const std::wstring& trace) noexcept
        {
            m_stackTrace = trace;
        }

        /// <summary>
        /// Managed exception type that the VM should instantiate. May be nullptr.
        /// </summary>
        TypeSymbol* exception_type() const noexcept
        {
            return m_exceptionType;
        }

        /// <summary>
        /// Narrow UTF-8 representation of the message, suitable for std::exception::what().
        /// </summary>
        const char* what() const noexcept override
        {
            if (m_narrowMessage.empty() && !m_message.empty())
            {
                // Best-effort conversion for diagnostic logging. Unusual code points are
                // replaced with a placeholder so the message remains a valid narrow string.
                for (wchar_t c : m_message)
                {
                    if (c <= 0x7F)
                    {
                        m_narrowMessage.push_back(static_cast<char>(c));
                    }
                    else
                    {
                        m_narrowMessage.push_back('?');
                    }
                }
            }

            return m_narrowMessage.c_str();
        }

    private:
        std::wstring m_message;
        std::wstring m_stackTrace;
        TypeSymbol* m_exceptionType = nullptr;
        mutable std::string m_narrowMessage;
    };

    /// <summary>
    /// Thrown by the FFI helper layer (CallState, argument unwrapping, view
    /// contracts) when foreign code does something strange, undefined or
    /// contract-violating. The VM maps it to the managed built-in
    /// UndefinedBehaviour class (see VirtualMachine::CreateRuntimeException),
    /// so scripts can catch it by type.
    /// </summary>
    class SHARD_API undefined_behaviour : public runtime_exception
    {
    public:
        using runtime_exception::runtime_exception;

        explicit undefined_behaviour(const std::string& message)
            : runtime_exception(Widen(message)) { }

    private:
        static std::wstring Widen(const std::string& message)
        {
            std::wstring result;
            result.reserve(message.size());
            for (char c : message)
                result.push_back(static_cast<unsigned char>(c) <= 0x7F ? static_cast<wchar_t>(c) : L'?');
            return result;
        }
    };
}
