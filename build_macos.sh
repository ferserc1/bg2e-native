#!/bin/bash

# Define project path and default scheme
PROJECT_PATH="Xcode/bg2e.xcodeproj"
DEFAULT_SCHEME="example-app"
CONFIGURATION="Release"

# Check if first argument is "run"
if [ "$1" == "run" ]; then
  SCHEME=$DEFAULT_SCHEME
  RUN_APP=1
else
  # Use first argument as scheme if provided, otherwise use default
  SCHEME=${1:-$DEFAULT_SCHEME}
  
  # Check if second argument is "run"
  if [ "$2" == "run" ]; then
    RUN_APP=1
  else
    RUN_APP=0
  fi
fi

# Print build information
echo "Building scheme: $SCHEME"
echo "Using project: $PROJECT_PATH"

# Build the project
xcodebuild -project "$PROJECT_PATH" -scheme "$SCHEME" -configuration $CONFIGURATION build -destination 'platform=macOS,arch=x86_64' -destination 'platform=macOS,arch=arm64'

# Check if build was successful
if [ $? -eq 0 ]; then
  echo "Build successful!"
  
  # Run the app if requested
  if [ $RUN_APP -eq 1 ]; then
    echo "Running $SCHEME..."
    # Get the app path from xcodebuild - usando grep más preciso para evitar PRECOMPS_INCLUDE_HEADERS_FROM_BUILT_PRODUCTS_DIR
    APP_PATH=$(xcodebuild -project "$PROJECT_PATH" -scheme "$SCHEME" -configuration $CONFIGURATION -showBuildSettings | grep "^ *BUILT_PRODUCTS_DIR =" | sed 's/^ *BUILT_PRODUCTS_DIR = //')
    APP_NAME=$(xcodebuild -project "$PROJECT_PATH" -scheme "$SCHEME" -configuration $CONFIGURATION -showBuildSettings | grep "^ *FULL_PRODUCT_NAME =" | sed 's/^ *FULL_PRODUCT_NAME = //')
    
    
    if [ -d "$APP_PATH/$APP_NAME" ]; then
      echo "Executing $APP_NAME with output to terminal..."
      # Ejecutar la aplicación directamente con salida a la terminal en lugar de usando 'open'
      "$APP_PATH/$APP_NAME/Contents/MacOS/$(echo $SCHEME | sed 's/ /_/g')"
    else
      echo "Could not find the application at $APP_PATH/$APP_NAME"
      exit 1
    fi
  fi
else
  echo "Build failed!"
  exit 1
fi
