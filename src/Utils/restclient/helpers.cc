/**
 * @file helpers.cpp
 * @brief implementation of the restclient helpers
 * @author Daniel Schauenberg <d@unwiredcouch.com>
 */
#include "helpers.h"

#include <cstring>

#include "restclient.h"

/**
 * @brief write callback function for libcurl
 *
 * @param data returned data of size (size*nmemb)
 * @param size size parameter
 * @param nmemb memblock parameter
 * @param userdata pointer to user data to save/work with return data
 *
 * @return (size * nmemb)
 */
size_t RestClient::Helpers::write_callback(void *data, size_t size,
                                           size_t nmemb, void *userdata) {
  RestClient::Response* r;
  r = reinterpret_cast<RestClient::Response*>(userdata);
  r->body.append(reinterpret_cast<char*>(data), size*nmemb);

  return (size * nmemb);
}

/**
 * @brief header callback for libcurl
 *
 * @param data returned (header line)
 * @param size of data
 * @param nmemb memblock
 * @param userdata pointer to user data object to save headr data
 * @return size * nmemb;
 */
size_t RestClient::Helpers::header_callback(void *data, size_t size,
                                            size_t nmemb, void *userdata) {
  RestClient::Response* r;
  r = reinterpret_cast<RestClient::Response*>(userdata);
  std::string header(reinterpret_cast<char*>(data), size*nmemb);
  size_t seperator = header.find_first_of(":");
  if ( std::string::npos == seperator ) {
    // roll with non seperated headers...
    trim(header);
    if (0 == header.length()) {
      return (size * nmemb);  // blank line;
    }
    r->headers[header] = "present";
  } else {
    std::string key = header.substr(0, seperator);
    trim(key);
    std::string value = header.substr(seperator + 1);
    trim(value);
    r->headers[key] = value;
  }

  return (size * nmemb);
}
size_t RestClient::Helpers::download_callback(void *ptr, size_t size, size_t nmemb, void *userdata)
{

	FILE *fp = reinterpret_cast<FILE*>(userdata);

	if (fp) {
		size_t nWrite = fwrite(ptr, size, nmemb, fp);
		return nWrite;
	}

	return (size * nmemb);
}

size_t RestClient::Helpers::progress_callback(void * clientp, int64_t  dltotal, int64_t  dlnow, int64_t  ultotal, int64_t  ulnow)
{
	static time_t last = 0; 
	time_t cur;
	time(&cur);
	typedef bool(*progress_cb)(double);
	progress_cb cb = (progress_cb)clientp;
	if (dlnow!= 0 && dltotal!=0 && dlnow == dltotal) {
		if (cb != nullptr)
			cb(100.0);
		return 0;
	}
	if ((cur - last) < 1)
		return 0;
	last = cur;
	if (dltotal == 0)
		return 0;
	
	
	if (cb != nullptr) {
		double progress = dlnow * 100.0 / dltotal;
		if (progress > 100.0)
			progress = 100.0;
		if (!cb(progress)) {
			return 1; //中断当前下载
		}
	}
	return 0;
}
/**
 * @brief read callback function for libcurl
 *
 * @param data pointer of max size (size*nmemb) to write data to
 * @param size size parameter
 * @param nmemb memblock parameter
 * @param userdata pointer to user data to read data from
 *
 * @return (size * nmemb)
 */
size_t RestClient::Helpers::read_callback(void *data, size_t size,
                                          size_t nmemb, void *userdata) {
  /** get upload struct */
  RestClient::Helpers::UploadObject* u;
  u = reinterpret_cast<RestClient::Helpers::UploadObject*>(userdata);
  /** set correct sizes */
  size_t curl_size = size * nmemb;
  size_t copy_size = (u->length < curl_size) ? u->length : curl_size;
  /** copy data to buffer */
  std::memcpy(data, u->data, copy_size);
  /** decrement length and increment data pointer */
  u->length -= copy_size;
  u->data += copy_size;
  /** return copied size */
  return copy_size;
}

size_t RestClient::Helpers::upload_read_callback(void * data, size_t size, size_t nmemb, void * userdata)
{
	FILE *fp = reinterpret_cast<FILE*>(userdata);

	if (fp) {
		size_t nRead = fread(data, size, nmemb, fp);
		return nRead;
	}

	return (size * nmemb);
}
