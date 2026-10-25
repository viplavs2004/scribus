# Bundled scrapbook libraries (open them in Windows > Scrapbook). Installed to <share>/scrapbook/
install(DIRECTORY "Ad Shapes" "Ad Shapes SVG"
  DESTINATION ${SHAREDIR}scrapbook
  PATTERN ".svn" EXCLUDE
)
