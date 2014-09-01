mkdir build\documentation\
doxygen simutrace\include\PublicApi.doxyfile
xcopy simutrace\documentation\theme build\documentation\html /E /Y