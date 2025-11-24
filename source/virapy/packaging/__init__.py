"""
virapy - Python bindings for Vira
"""

import os
import sys
import platform

# Import version from generated file
try:
    from ._version import __version__
except ImportError:
    __version__ = "unknown"

# Get the directory where this __init__.py file is located
_package_dir = os.path.dirname(os.path.abspath(__file__))

# On Windows, add DLL search path
if platform.system() == "Windows":
    if hasattr(os, 'add_dll_directory'):
        os.add_dll_directory(_package_dir)
    else:
        os.environ['PATH'] = _package_dir + os.pathsep + os.environ.get('PATH', '')

# Global variable to store the loaded extension
_extension_module = None

def _load_extension():
    """Load the C++ extension module if not already loaded"""
    global _extension_module
    
    if _extension_module is not None:
        return _extension_module
    
    try:
        # Check that the extension file exists
        if platform.system() == "Windows":
            extension_file = os.path.join(_package_dir, "virapy.pyd")
        else:
            extension_file = os.path.join(_package_dir, "virapy.so")
        
        if not os.path.exists(extension_file):
            raise ImportError(f"Extension file not found: {extension_file}")
        
        # The trick: temporarily rename ourselves in sys.modules to avoid conflict
        import importlib.util
        
        # Store reference to this module
        this_module = sys.modules[__name__]
        
        # Temporarily remove ourselves from sys.modules
        temp_name = __name__ + "_temp"
        sys.modules[temp_name] = this_module
        if __name__ in sys.modules:
            del sys.modules[__name__]
        
        try:
            # Now we can import the extension with the same name
            spec = importlib.util.spec_from_file_location(__name__, extension_file)
            if spec and spec.loader:
                extension_module = importlib.util.module_from_spec(spec)
                spec.loader.exec_module(extension_module)
                _extension_module = extension_module
                return extension_module
            else:
                raise ImportError(f"Could not create spec for {extension_file}")
                
        finally:
            # Restore ourselves in sys.modules
            sys.modules[__name__] = this_module
            if temp_name in sys.modules:
                del sys.modules[temp_name]
            
    except Exception as e:
        raise ImportError(f"Failed to import virapy extension: {e}")

class _LazySubmodule:
    """Lazy loader for submodules"""
    def __init__(self, name):
        self.name = name
        self._submodule = None
        self._activated = False
    
    def _ensure_activated(self):
        """Ensure this submodule is activated before use"""
        if not self._activated:
            extension = _load_extension()
            self._submodule = getattr(extension, self.name)
            # Call the activation function to enforce exclusivity
            self._submodule._activate()
            self._activated = True
    
    def __getattr__(self, attr):
        self._ensure_activated()
        return getattr(self._submodule, attr)
    
    def __dir__(self):
        self._ensure_activated()
        return dir(self._submodule)

# Create lazy loaders for each submodule
V1FF = _LazySubmodule("V1FF")
V8FF = _LazySubmodule("V8FF")
RGBFF = _LazySubmodule("RGBFF")

# Make them available for import
__all__ = ["V1FF", "V8FF", "RGBFF", "__version__"]