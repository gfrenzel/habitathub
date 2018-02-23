@ECHO OFF
SETLOCAL ENABLEEXTENSIONS

set lambda_name=Alexa_HabHub_Custom_Skill
set zip_file=%lambda_name%.zip
7z.exe a -tzip %zip_file% -r -x!*.zip -x!*.cmd

REM ECHO Updating Function Configuration
REM call aws lambda update-function-configuration --function-name "%lambda_name%" --handler "index.handler"

REM ECHO %ERRORLEVEL%
REM IF %ERRORLEVEL% NEQ 0 (
REM     ECHO Error - Failed update-func-config error code %ERRORLEVEL%
REM     EXIT
REM )

REM ECHO Updating Function Code
REM call aws lambda update-function-code --function-name "%lambda_name%" --zip-file "fileb://%zip_file%"

REM IF %ERRORLEVEL% NEQ 0 (
REM     ECHO Error - Failed update-func-code error code %ERRORLEVEL%
REM     EXIT
REM )

REM ECHO Deployed.

