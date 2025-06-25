#include <functional>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

template <typename ValueType, typename ErrorType>
class Result {
public:
    template <typename T>
    static Result Success(T&& value)
    {
        return Result(std::forward<T>(value));
    }
    template <typename E>
    static Result Failure(E&& error)
    {
        return Result(std::forward<E>(error));
    }

public:
    template <typename U>
    Result<U, ErrorType> Bind(std::function<Result<U, ErrorType>(ValueType)> func) const
    {
        if (mIsSuccess) {
            return func(mValue);
        } else {
            return Result<U, ErrorType>::Failure(mError);
        }
    }

    // Use this for better performance
    template <typename Func>
    auto Bind(Func func) const
        -> decltype(func(std::declval<ValueType>()))
    {
        using ResultU = decltype(func(std::declval<ValueType>()));
        if (mIsSuccess) {
            return func(mValue);
        } else {
            return ResultU::Failure(mError);
        }
    }

public:
    bool IsSuccess() const { return mIsSuccess; }
    const ValueType& Value() const { return mValue; }
    const ErrorType& Error() const { return mError; }

private:
    template <typename T, std::enable_if_t<std::is_convertible_v<T, ValueType>, int> = 0>
    Result(T&& value)
        : mIsSuccess(true)
        , mValue(std::forward<T>(value))
        , mError()
    {
    }
    template <typename E, std::enable_if_t<std::is_convertible_v<E, ErrorType>, int> = 0>
    Result(E&& error)
        : mIsSuccess(false)
        , mValue()
        , mError(std::forward<E>(error))
    {
    }

    bool mIsSuccess;
    ValueType mValue;
    ErrorType mError;
};

template <typename ValueType, typename ErrorType>
class MutableResult {
public:
    using ValuePtr = std::shared_ptr<ValueType>;

    static MutableResult Success(const ValueType& value)
    {
        return MutableResult(std::make_shared<ValueType>(value), {}, true);
    }

    static MutableResult Failure(const ErrorType& error)
    {
        return MutableResult(nullptr, error, false);
    }

public:
    template <typename Func>
    MutableResult& InPlaceBind(Func func)
    {
        if (mIsSuccess && mValue) {
            func(*mValue);
        }
        return *this;
    }

    template <typename Func>
    const MutableResult& ReadOnlyBind(Func func) const
    {
        if (mIsSuccess && mValue) {
            func(*mValue);
        }
        return *this;
    }

public:
    const ValueType& Value() const { return *mValue; }
    const ErrorType& Error() const { return mError; }
    bool IsSuccess() const { return mIsSuccess; }

private:
    MutableResult(ValuePtr value, const ErrorType& error, bool isSuccess)
        : mValue(value)
        , mError(error)
        , mIsSuccess(isSuccess)
    {
    }

    ValuePtr mValue;
    ErrorType mError;
    bool mIsSuccess;
};

struct Manuscript {
    int id;
    std::string title;
    std::string content;
    std::string author;
    std::time_t submission_date;
};

struct EditedManuscript {
    int id;
    std::string title;
    std::string content;
    std::string author;
    std::vector<std::string> editorial_notes;
    std::time_t edit_date;
};

struct FormattedManuscript {
    int id;
    std::string title;
    std::string formatted_content;
    std::string author;
    std::string format_type;
    std::time_t format_date;
};

struct ReviewedManuscript {
    int id;
    std::string title;
    std::string formatted_content;
    std::string author;
    bool approved;
    std::vector<std::string> review_comments;
    std::time_t review_date;
};

struct PublishedManuscript {
    int id;
    std::string title;
    std::string formatted_content;
    std::string author;
    std::string isbn;
    std::time_t publish_date;
};

Result<Manuscript, std::string> FetchManuscript(int manuscriptId)
{
    if (manuscriptId <= 0) {
        return Result<Manuscript, std::string>::Failure("Invalid manuscript ID");
    }

    Manuscript m = {
        manuscriptId,
        "The Art of Programming",
        "Initial content...",
        "John Doe",
        std::time(nullptr)
    };
    return Result<Manuscript, std::string>::Success(m);
}

Result<EditedManuscript, std::string> EditManuscript(Manuscript m)
{
    if (m.content.empty()) {
        return Result<EditedManuscript, std::string>::Failure("Empty manuscript content");
    }

    EditedManuscript em = {
        m.id,
        m.title,
        m.content + "\nEdited content...",
        m.author,
        { "Fixed grammar", "Improved structure" },
        std::time(nullptr)
    };
    return Result<EditedManuscript, std::string>::Success(em);
}

Result<FormattedManuscript, std::string> FormatManuscript(EditedManuscript em)
{
    if (em.editorial_notes.empty()) {
        return Result<FormattedManuscript, std::string>::Failure("No editorial notes found");
    }

    FormattedManuscript fm = {
        em.id,
        em.title,
        em.content + "\nFormatted according to style guide...",
        em.author,
        "IEEE",
        std::time(nullptr)
    };
    return Result<FormattedManuscript, std::string>::Success(fm);
}

Result<ReviewedManuscript, std::string> ReviewManuscript(FormattedManuscript fm)
{
    if (fm.format_type != "IEEE") {
        return Result<ReviewedManuscript, std::string>::Failure("Invalid format type");
    }

    ReviewedManuscript rm = {
        fm.id,
        fm.title,
        fm.formatted_content,
        fm.author,
        true,
        { "Excellent work", "Ready for publication" },
        std::time(nullptr)
    };
    return Result<ReviewedManuscript, std::string>::Success(rm);
}

Result<PublishedManuscript, std::string> PublishManuscript(ReviewedManuscript rm)
{
    if (!rm.approved) {
        return Result<PublishedManuscript, std::string>::Failure("Manuscript not approved");
    }

    PublishedManuscript pm = {
        rm.id,
        rm.title,
        rm.formatted_content,
        rm.author,
        "ISBN-" + std::to_string(rm.id) + "-2023",
        std::time(nullptr)
    };
    return Result<PublishedManuscript, std::string>::Success(pm);
}

int main()
{
    int manuscriptId = 0;
    std::cout << "Enter manuscript ID: ";
    std::cin >> manuscriptId;

    auto publishingPipeline = FetchManuscript(manuscriptId)
                                  .Bind(EditManuscript)
                                  .Bind(FormatManuscript)
                                  .Bind(ReviewManuscript)
                                  .Bind(PublishManuscript);

    if (publishingPipeline.IsSuccess()) {
        const auto& result = publishingPipeline.Value();
        std::cout << "Successfully published!\n"
                  << "Title: " << result.title << "\n"
                  << "Author: " << result.author << "\n"
                  << "ISBN: " << result.isbn << "\n"
                  << "Publish Date: " << std::ctime(&result.publish_date);
    } else {
        std::cout << "Error in publishing pipeline: " << publishingPipeline.Error() << std::endl;
    }

    return 0;
}