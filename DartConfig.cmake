# Dashboard is opened for submissions for a 24 hour period starting at
# the specified NIGHLY_START_TIME. Time is specified in 24 hour format.
SET (NIGHTLY_START_TIME "0:30:00 EDT")

# Dart server to submit results (used by client)
SET (DROP_SITE "public.kitware.com")
SET (DROP_LOCATION "/incoming")
SET (DROP_SITE_USER "ftpuser")
SET (DROP_SITE_PASSWORD "public")
SET (TRIGGER_SITE 
       "http://${DROP_SITE}/cgi-bin/Submit-Xdmf-TestingResults.pl")

# Project Home Page
SET (PROJECT_URL "http://www.arl.hpc.mil/ice/")

# Dart server configuration 
SET (ROLLUP_URL "http://${DROP_SITE}/cgi-bin/xdmf-rollup-dashboard.sh")
SET (CVS_WEB_URL "http://${DROP_SITE}/cgi-bin/cvsweb.cgi/Xdmf/")
SET (CVS_WEB_CVSROOT "Xdmf")
#SET (DOXYGEN_URL "http://${DROP_SITE}/" )
#SET (GNATS_WEB_URL "http://${DROP_SITE}/")

# copy over the testing logo
#CONFIGURE_FILE(${PARAVIEW_SOURCE_DIR}/Web/Art/ParaViewLogo.gif ${PARAVIEW_BINARY_DIR}/Testing/HTML/TestingResults/Icons/Logo.gif COPYONLY)
