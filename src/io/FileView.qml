import Quickshell.Io

FileViewInternal {
	property bool preload: this.__preload;
	property bool blockLoading: this.__blockLoading;
	property bool blockAllReads: this.__blockAllReads;
	property bool printErrors: this.__printErrors;
	property string path: this.__path;

	onPreloadChanged: this.__preload = preload;
	onBlockLoadingChanged: this.__blockLoading = this.blockLoading;
	onBlockAllReadsChanged: this.__blockAllReads = this.blockAllReads;
	onPrintErrorsChanged: this.__printErrors = this.printErrors;

	// Unfortunately path can't be kept as an empty string until the file loads
	// without using QQmlPropertyValueInterceptor which is private. If we lean fully
	// into using private code in the future, there will be no reason not to do it here.

	onPathChanged: {
		if (!this.preload) this.__preload = false;
		this.__printErrors = this.printErrors;
		this.__path = this.path;
		if (this.preload) this.__preload = true;
	}

	// The C++ side can't force bindings to be resolved in a specific order so
	// its done here. Functions are used to avoid the eager loading aspect of properties.

	// Preload is set as it is below to avoid starting an async read from a preload
	// if the user wants an initial blocking read.

	function text(): string {
		if (!this.preload) this.__preload = false;
		this.__blockLoading = this.blockLoading;
		this.__blockAllReads = this.blockAllReads;
		this.__printErrors = this.printErrors;
		this.__path = this.path;
		const text = this.__text;
		if (this.preload) this.__preload = true;
		return text;
	}

	function data(): var {
		if (!this.preload) this.__preload = false;
		this.__blockLoading = this.blockLoading;
		this.__blockAllReads = this.blockAllReads;
		this.__printErrors = this.printErrors;
		this.__path = this.path;
		const data = this.__data;
		if (this.preload) this.__preload = true;
		return data;
	}
}
