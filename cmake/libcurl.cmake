if(WIN32)
    set(CURL_LIBRARIES libcurl libssl libcrypto zlib)
endif()

# Temporarily skip CURL for compilation
# find_package(CURL REQUIRED)
set(CURL_FOUND OFF)
set(CURL_LIBRARIES "")
set(CURL_INCLUDE_DIRS "")

if(ANDROID)
    find_package(OpenSSL REQUIRED)
    set(CURL_LIBRARIES CURL::libcurl OpenSSL::SSL OpenSSL::Crypto)
endif()
