mkdir build\documentation\
doxygen simutrace\documentation\PublicApi.doxyfile
xcopy simutrace\documentation\theme build\documentation\html /E /Y