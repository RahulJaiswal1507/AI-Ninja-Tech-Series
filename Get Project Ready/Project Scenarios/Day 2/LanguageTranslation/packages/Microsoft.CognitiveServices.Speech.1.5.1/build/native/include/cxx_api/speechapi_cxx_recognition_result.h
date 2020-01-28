//
// Copyright (c) Microsoft. All rights reserved.
// See https://aka.ms/csspeech/license201809 for the full license information.
//
// speechapi_cxx_recognition_result.h: Public API declarations for RecognitionResult C++ base class and related enum class
//

#pragma once
#include <string>
#include <speechapi_cxx_common.h>
#include <speechapi_cxx_string_helpers.h>
#include <speechapi_cxx_enums.h>
#include <speechapi_cxx_properties.h>
#include <speechapi_c_result.h>


namespace Microsoft {
namespace CognitiveServices {
namespace Speech {


/// <summary>
/// Contains detailed information about result of a recognition operation.
/// </summary>
class RecognitionResult
{
private:

    /*! \cond PRIVATE */

    class PrivatePropertyCollection : public PropertyCollection
    {
    public:
        PrivatePropertyCollection(SPXRESULTHANDLE hresult) :
            PropertyCollection(
                [=]() {
                SPXPROPERTYBAGHANDLE hpropbag = SPXHANDLE_INVALID;
                result_get_property_bag(hresult, &hpropbag);
                return hpropbag;
            }())
        {
        }
    };

    PrivatePropertyCollection m_properties;

    /*! \endcond */

public:

    /// <summary>
    /// Virtual destructor.
    /// </summary>
    virtual ~RecognitionResult()
    {
        ::recognizer_result_handle_release(m_hresult);
        m_hresult = SPXHANDLE_INVALID;
    };

    /// <summary>
    /// Unique result id.
    /// </summary>
    const SPXSTRING& ResultId;

    /// <summary>
    /// Recognition reason.
    /// </summary>
    const Speech::ResultReason& Reason;

    /// <summary>
    /// Normalized text generated by a speech recognition engine from recognized input.
    /// </summary>
    const SPXSTRING& Text;

    /// <summary>
    /// Duration of recognized speech in ticks.
    /// A single tick represents one hundred nanoseconds or one ten-millionth of a second.
    /// </summary>
    /// <returns>Duration of recognized speech in ticks.</returns>
    uint64_t Duration() const { return m_duration; }

    /// <summary>
    /// Offset of the recognized speech in ticks.
    /// A single tick represents one hundred nanoseconds or one ten-millionth of a second.
    /// </summary>
    /// <returns>Offset of the recognized speech in ticks.</returns>
    uint64_t Offset() const { return m_offset; }

    /// <summary>
    /// Collection of additional RecognitionResult properties.
    /// </summary>
    const PropertyCollection& Properties;

    /// <summary>
    /// Internal. Explicit conversion operator.
    /// </summary>
    explicit operator SPXRESULTHANDLE() { return m_hresult; }


protected:

    /*! \cond PROTECTED */

    explicit RecognitionResult(SPXRESULTHANDLE hresult) :
        m_properties(hresult),
        ResultId(m_resultId),
        Reason(m_reason),
        Text(m_text),
        Properties(m_properties),
        Handle(m_hresult),
        m_hresult(hresult)
    {
        PopulateResultFields(hresult, &m_resultId, &m_reason, &m_text);
    }

    const SPXRESULTHANDLE& Handle;

    /*! \endcond */

private:

    DISABLE_DEFAULT_CTORS(RecognitionResult);

    void PopulateResultFields(SPXRESULTHANDLE hresult, SPXSTRING* resultId, Speech::ResultReason* reason, SPXSTRING* text)
    {

        SPX_INIT_HR(hr);

        const size_t maxCharCount = 1024;
        char sz[maxCharCount + 1];

        if (resultId != nullptr)
        {
            SPX_THROW_ON_FAIL(hr = result_get_result_id(hresult, sz, maxCharCount));
            *resultId = Utils::ToSPXString(sz);
        }

        if (reason != nullptr)
        {
            Result_Reason resultReason;
            SPX_THROW_ON_FAIL(hr = result_get_reason(hresult, &resultReason));
            *reason = (Speech::ResultReason)resultReason;
        }

        if (text != nullptr)
        {
            SPX_THROW_ON_FAIL(hr = result_get_text(hresult, sz, maxCharCount));
            *text = Utils::ToSPXString(sz);
        }

        SPX_THROW_ON_FAIL(hr = result_get_offset(hresult, &m_offset));
        SPX_THROW_ON_FAIL(hr = result_get_duration(hresult, &m_duration));
    }

    SPXRESULTHANDLE m_hresult;

    SPXSTRING m_resultId;
    Speech::ResultReason m_reason;
    SPXSTRING m_text;
    uint64_t m_offset;
    uint64_t m_duration;
};


/// <summary>
/// Contains detailed information about why a result was canceled.
/// </summary>
class CancellationDetails
{
private:

    CancellationReason m_reason;
    CancellationErrorCode m_errorCode;

public:

    /// <summary>
    /// Creates an instance of CancellationDetails object for the canceled RecognitionResult.
    /// </summary>
    /// <param name="result">The result that was canceled.</param>
    /// <returns>A shared pointer to CancellationDetails.</returns>
    static std::shared_ptr<CancellationDetails> FromResult(std::shared_ptr<RecognitionResult> result)
    {
        // VSTS 1407221
        // SPX_THROW_HR_IF(result->Reason != ResultReason::Canceled, SPXERR_INVALID_ARG);
        auto ptr = new CancellationDetails(result.get());
        auto cancellation = std::shared_ptr<CancellationDetails>(ptr);
        return cancellation;
    }

    /// <summary>
    /// The reason the result was canceled.
    /// </summary>
    const CancellationReason& Reason;

    /// <summary>
    /// The error code in case of an unsuccessful recognition (<see cref="Reason"/> is set to Error).
    /// If Reason is not Error, ErrorCode is set to NoError.
    /// Added in version 1.1.0.
    /// </summary>
    const CancellationErrorCode& ErrorCode;

    /// <summary>
    /// The error message in case of an unsuccessful recognition (<see cref="Reason"/> is set to Error).
    /// </summary>
    const SPXSTRING ErrorDetails;

protected:

    /*! \cond PROTECTED */

    CancellationDetails(RecognitionResult* result) :
        m_reason(GetCancellationReason(result)),
        m_errorCode(GetCancellationErrorCode(result)),
        Reason(m_reason),
        ErrorCode(m_errorCode),
        ErrorDetails(result->Properties.GetProperty(PropertyId::SpeechServiceResponse_JsonErrorDetails))
    {
    }

    /*! \endcond */

private:

    DISABLE_DEFAULT_CTORS(CancellationDetails);

    Speech::CancellationReason GetCancellationReason(RecognitionResult* result)
    {
        Result_CancellationReason reason;

        SPXRESULTHANDLE hresult = (SPXRESULTHANDLE)(*result);
        SPX_IFFAILED_THROW_HR(result_get_reason_canceled(hresult, &reason));

        return (Speech::CancellationReason)reason;
    }

    Speech::CancellationErrorCode GetCancellationErrorCode(RecognitionResult* result)
    {
        Result_CancellationErrorCode errorCode;

        SPXRESULTHANDLE hresult = (SPXRESULTHANDLE)(*result);
        SPX_IFFAILED_THROW_HR(result_get_canceled_error_code(hresult, &errorCode));

        return (Speech::CancellationErrorCode)errorCode;
    }

};


/// <summary>
/// Contains detailed information for NoMatch recognition results.
/// </summary>
class NoMatchDetails
{
private:

    NoMatchReason m_reason;

public:

    /// <summary>
    /// Creates an instance of NoMatchDetails object for NoMatch RecognitionResults.
    /// </summary>
    /// <param name="result">The recognition result that was not recognized.</param>
    /// <returns>A shared pointer to NoMatchDetails.</returns>
    static std::shared_ptr<NoMatchDetails> FromResult(std::shared_ptr<RecognitionResult> result)
    {
        // VSTS 1407221
        // SPX_IFTRUE_THROW_HR(result->Reason != ResultReason::NoMatch, SPXERR_INVALID_ARG);
        auto ptr = new NoMatchDetails(result.get());
        auto noMatch = std::shared_ptr<NoMatchDetails>(ptr);
        return noMatch;
    }

    /// <summary>
    /// The reason the result was not recognized.
    /// </summary>
    const NoMatchReason& Reason;

protected:

    /*! \cond PROTECTED */

    NoMatchDetails(RecognitionResult* result) :
        m_reason(GetNoMatchReason(result)),
        Reason(m_reason)
    {
    }

    /*! \endcond */

private:

    DISABLE_DEFAULT_CTORS(NoMatchDetails);

    Speech::NoMatchReason GetNoMatchReason(RecognitionResult* result)
    {
        Result_NoMatchReason reason;

        SPXRESULTHANDLE hresult = (SPXRESULTHANDLE)(*result);
        SPX_IFFAILED_THROW_HR(result_get_no_match_reason(hresult, &reason));

        return (Speech::NoMatchReason)reason;
    }

};

} } } // Microsoft::CognitiveServices::Speech