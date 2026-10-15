
class Error {
public:
	enum Code {
		Success,
		Full,
		Empty,
		NoEnoughMemory,
		IndexOutOfRange,
		HostControllerNotHalted,
		InvalidSlotID,
		PortNotConnected,
		InvalidEndpointNumber,
		TransferRingNotSet,
		AlreadyAllocated,
		NotImplemented,
		InvalidDescriptor,
		BufferTooSmall,
		UnknownDevice,
		NoCorrespondingSetupStage,
		TransferFailed,
		InvalidPhase,
		UnknownXHCISpeedID,
		NoWaiter,
		UnsupportedXHCIVersion,
		InvalidControllerCapability,
		UnsupportedPageSize,
		//
		LastOfCode
	};

private:
	static constexpr uni::Array code_names_{
	"Success",
	"Full",
	"Empty",
	"NoEnoughMemory",
	"IndexOutOfRange",
	"HostControllerNotHalted",
	"InvalidSlotID",
	"PortNotConnected",
	"InvalidEndpointNumber",
	"TransferRingNotSet",
	"AlreadyAllocated",
	"NotImplemented",
	"InvalidDescriptor",
	"BufferTooSmall",
	"UnknownDevice",
	"NoCorrespondingSetupStage",
	"TransferFailed",
	"InvalidPhase",
	"UnknownXHCISpeedID",
	"NoWaiter",
	"UnsupportedXHCIVersion",
	"InvalidControllerCapability",
	"UnsupportedPageSize",
	};
	static_assert(Error::Code::LastOfCode == code_names_.size());

public:
	Error(Code code, const char* file, int line) : code_{ code }, line_{ line }, file_{ file } {}

	Code Cause() const {
		return this->code_;
	}

	operator bool() const {
		return this->code_ != Success;
	}

	const char* Name() const {
		return code_names_[static_cast<int>(this->code_)];
	}

	const char* File() const {
		return this->file_;
	}

	int Line() const {
		return this->line_;
	}

private:
	Code code_;
	int line_;
	const char* file_;
};

#define MAKE_ERROR(code) Error((code), __FILE__, __LINE__)

template <class T>
struct WithError {
	T value;
	Error error;
};
