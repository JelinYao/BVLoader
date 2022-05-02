/**
 * @file connection.cpp
 * @brief implementation of the connection class
 * @author Daniel Schauenberg <d@unwiredcouch.com>
 */
#include "connection.h"

#include <curl/curl.h>

#include <cstring>
#include <string>
#include <iostream>
#include <map>
#include <stdexcept>

#include "restclient.h"
#include "helpers.h"

/**
 * @brief constructor for the Connection object
 *
 * @param baseUrl - base URL for the connection to use
 *
 */
RestClient::Connection::Connection()
                               : lastRequest(), headerFields() {
  this->curlHandle = curl_easy_init();
  if (!this->curlHandle) {
    throw std::runtime_error("Couldn't initialize curl handle");
  }
  this->baseUrl = baseUrl;
  this->timeout = 3600;
  this->connectTimeout = 10;
  this->followRedirects = true;
}

RestClient::Connection::~Connection() {
  if (this->curlHandle) {
    curl_easy_cleanup(this->curlHandle);
  }
}

// getters/setters

/**
 * @brief get diagnostic information about the connection object
 *
 * @return RestClient::Connection::Info struct
 */
RestClient::Connection::Info
RestClient::Connection::GetInfo() {
  RestClient::Connection::Info ret;
  ret.baseUrl = this->baseUrl;
  ret.headers = this->GetHeaders();
  ret.timeout = this->timeout;
  ret.basicAuth.username = this->basicAuth.username;
  ret.basicAuth.password = this->basicAuth.password;
  ret.customUserAgent = this->customUserAgent;
  ret.lastRequest = this->lastRequest;

  return ret;
}

/**
 * @brief append a header to the internal map
 *
 * @param key for the header field
 * @param value for the header field
 *
 */
void
RestClient::Connection::AppendHeader(const std::string& key,
                                     const std::string& value) {
  this->headerFields[key] = value;
}

/**
 * @brief set the custom headers map. This will replace the currently
 * configured headers with the provided ones. If you want to add additional
 * headers, use AppendHeader()
 *
 * @param headers to set
 */
void
RestClient::Connection::SetHeaders(RestClient::HeaderFields headers) {
  this->headerFields = headers;
}

/**
 * @brief get all custom headers set on the connection
 *
 * @returns a RestClient::HeaderFields map containing the custom headers
 */
RestClient::HeaderFields
RestClient::Connection::GetHeaders() {
  return this->headerFields;
}

/**
 * @brief configure whether to follow redirects on this connection
 *
 * @param follow - boolean whether to follow redirects
 */
void
RestClient::Connection::FollowRedirects(bool follow) {
  this->followRedirects = follow;
}

/**
 * @brief set custom user agent for connection. This gets prepended to the
 * default restclient-cpp/RESTCLIENT_VERSION string
 *
 * @param userAgent - custom userAgent prefix
 *
 */
void
RestClient::Connection::SetUserAgent(const std::string& userAgent) {
  this->customUserAgent = userAgent;
}

/**
 * @brief set custom Certificate Authority (CA) path
 *
 * @param caInfoFilePath - The path to a file holding the certificates used to
 * verify the peer with. See CURLOPT_CAINFO
 *
 */
void
RestClient::Connection::SetCAInfoFilePath(const std::string& caInfoFilePath) {
  this->caInfoFilePath = caInfoFilePath;
}

/**
 * @brief get the user agent to add to the request
 *
 * @return user agent as std::string
 */
std::string
RestClient::Connection::GetUserAgent() {
  std::string prefix;
  if (this->customUserAgent.length() > 0) {
    prefix = this->customUserAgent + " ";
  }
  return std::string(prefix + "dbhack");
}

/**
 * @brief set timeout for connection
 *
 * @param seconds - timeout in seconds
 *
 */
void
RestClient::Connection::SetTimeout(int seconds) {
  this->timeout = seconds;
}

void RestClient::Connection::SetConnectTimeout(int seconds)
{
	this->connectTimeout = seconds;
}

/**
 * @brief set username and password for basic auth
 *
 * @param username
 * @param password
 *
 */
void
RestClient::Connection::SetBasicAuth(const std::string& username,
                                     const std::string& password) {
  this->basicAuth.username = username;
  this->basicAuth.password = password;
}

/**
 * @brief helper function to get called from the actual request methods to
 * prepare the curlHandle for transfer with generic options, perform the
 * request and record some stats from the last request and then reset the
 * handle with curl_easy_reset to its default state. This will keep things
 * like connections and session ID intact but makes sure you can change
 * parameters on the object for another request.
 *
 * @param uri URI to query
 * @param ret Reference to the Response struct that should be filled
 *
 * @return 0 on success and 1 on error
 */
RestClient::Response
RestClient::Connection::performCurlRequest(const std::string& uri) {
  // init return type
  RestClient::Response ret = {};

  std::string url = std::string(this->baseUrl + uri);
  std::string headerString;
  CURLcode res = CURLE_OK;
  curl_slist* headerList = NULL;

  /** set query URL */
  curl_easy_setopt(this->curlHandle, CURLOPT_URL, url.c_str());
  /** set callback function */
  curl_easy_setopt(this->curlHandle, CURLOPT_WRITEFUNCTION,
                   Helpers::write_callback);
  /** set data object to pass to callback function */
  curl_easy_setopt(this->curlHandle, CURLOPT_WRITEDATA, &ret);
  /** set the header callback function */
  curl_easy_setopt(this->curlHandle, CURLOPT_HEADERFUNCTION,
                   Helpers::header_callback);
  /** callback object for headers */
  curl_easy_setopt(this->curlHandle, CURLOPT_HEADERDATA, &ret);

  //curl_easy_setopt(this->curlHandle, CURLOPT_CAINFO, "cacert.pem");
  curl_easy_setopt(this->curlHandle, CURLOPT_SSL_VERIFYPEER, 0);
  curl_easy_setopt(this->curlHandle, CURLOPT_SSL_VERIFYHOST, 0);

  curl_easy_setopt(this->curlHandle, CURLOPT_VERBOSE, 1L);

 // curl_easy_setopt(this->curlHandle, CURLOPT_PROXY, "http://127.0.0.1:8888");
  /** set http headers */
  for (HeaderFields::const_iterator it = this->headerFields.begin();
      it != this->headerFields.end(); ++it) {
    headerString = it->first;
    headerString += ": ";
    headerString += it->second;
    headerList = curl_slist_append(headerList, headerString.c_str());
  }
  curl_easy_setopt(this->curlHandle, CURLOPT_HTTPHEADER,
      headerList);

  // set basic auth if configured
  if (this->basicAuth.username.length() > 0) {
    std::string authString = std::string(this->basicAuth.username + ":" +
                                         this->basicAuth.password);
    curl_easy_setopt(this->curlHandle, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
    curl_easy_setopt(this->curlHandle, CURLOPT_USERPWD, authString.c_str());
  }
  /** set user agent */
  curl_easy_setopt(this->curlHandle, CURLOPT_USERAGENT,
                   this->GetUserAgent().c_str());

  // set timeout
  if (this->timeout) {
    curl_easy_setopt(this->curlHandle, CURLOPT_TIMEOUT, this->timeout);
    // dont want to get a sig alarm on timeout
    curl_easy_setopt(this->curlHandle, CURLOPT_NOSIGNAL, 1);
  }
  if (this->connectTimeout) {
	  curl_easy_setopt(this->curlHandle, CURLOPT_CONNECTTIMEOUT, this->connectTimeout);
  }
  // set follow redirect
  if (this->followRedirects == true) {
    curl_easy_setopt(this->curlHandle, CURLOPT_FOLLOWLOCATION, 1L);
  }
  // if provided, supply CA path
  if (!this->caInfoFilePath.empty()) {
    curl_easy_setopt(this->curlHandle, CURLOPT_CAINFO,
                     this->caInfoFilePath.c_str());
  }
  else {
	  curl_easy_setopt(this->curlHandle, CURLOPT_SSL_VERIFYPEER, 0);
	  curl_easy_setopt(this->curlHandle, CURLOPT_SSL_VERIFYHOST, 0);
  }
  res = curl_easy_perform(this->curlHandle);
  ret.curl_code = res;
  if (res != CURLE_OK) 
  {
	  const char *msg = curl_easy_strerror(res);
	  ret.body = (msg != NULL) ? msg : "unknow error";
  } 
  else 
  {
    int64_t http_code = 0;
    curl_easy_getinfo(this->curlHandle, CURLINFO_RESPONSE_CODE, &http_code);
    ret.http_code = static_cast<int>(http_code);
  }

  curl_easy_getinfo(this->curlHandle, CURLINFO_TOTAL_TIME,
                    &this->lastRequest.totalTime);
  curl_easy_getinfo(this->curlHandle, CURLINFO_NAMELOOKUP_TIME,
                    &this->lastRequest.nameLookupTime);
  curl_easy_getinfo(this->curlHandle, CURLINFO_CONNECT_TIME,
                    &this->lastRequest.connectTime);
  curl_easy_getinfo(this->curlHandle, CURLINFO_APPCONNECT_TIME,
                    &this->lastRequest.appConnectTime);
  curl_easy_getinfo(this->curlHandle, CURLINFO_PRETRANSFER_TIME,
                    &this->lastRequest.preTransferTime);
  curl_easy_getinfo(this->curlHandle, CURLINFO_STARTTRANSFER_TIME,
                    &this->lastRequest.startTransferTime);
  curl_easy_getinfo(this->curlHandle, CURLINFO_REDIRECT_TIME,
                    &this->lastRequest.redirectTime);
  curl_easy_getinfo(this->curlHandle, CURLINFO_REDIRECT_COUNT,
                    &this->lastRequest.redirectCount);
  // free header list
  curl_slist_free_all(headerList);
  // reset curl handle
  curl_easy_reset(this->curlHandle);
  return ret;
}

RestClient::Response RestClient::Connection::performCurlRequest(const std::string& uri, const std::string& filename, progress_cb cb)
{
	RestClient::Response ret;
	FILE *fp = NULL;
	fopen_s(&fp, filename.c_str(), "wb+");
	if (NULL == fp)
	{
		//LOGGER_ERROR << "���ļ���" << filename << "ʧ�ܣ�ϵͳ�����룺" << GetLastError();
		return ret;
	}

	std::string url = std::string(this->baseUrl + uri);
	std::string headerString;
	CURLcode res = CURLE_OK;
	curl_slist* headerList = NULL;

	/** set query URL */
	curl_easy_setopt(this->curlHandle, CURLOPT_URL, url.c_str());
	/** set callback function */
	curl_easy_setopt(this->curlHandle, CURLOPT_WRITEFUNCTION,
		Helpers::download_callback);
	/** set data object to pass to callback function */
	curl_easy_setopt(this->curlHandle, CURLOPT_WRITEDATA, fp);
	/** set the header callback function */
	curl_easy_setopt(this->curlHandle, CURLOPT_HEADERFUNCTION,
		Helpers::header_callback);
	/** callback object for headers */
	curl_easy_setopt(this->curlHandle, CURLOPT_HEADERDATA, &ret);

	//if (cb != nullptr) {
	curl_easy_setopt(this->curlHandle, CURLOPT_NOPROGRESS, 0L);

	curl_easy_setopt(this->curlHandle, CURLOPT_XFERINFOFUNCTION, Helpers::progress_callback);
		/* pass the struct pointer into the xferinfo function, note that this is
		an alias to CURLOPT_PROGRESSDATA */
	if (cb != nullptr) {
		curl_easy_setopt(this->curlHandle, CURLOPT_XFERINFODATA, cb);
	}
	else {
		//curl_easy_setopt(this->curlHandle, CURLOPT_XFERINFODATA, Helpers::default_download_callback);
	}
	
	//}
	/** set http headers */
	for (HeaderFields::const_iterator it = this->headerFields.begin();
		it != this->headerFields.end(); ++it) {
		headerString = it->first;
		headerString += ": ";
		headerString += it->second;
		headerList = curl_slist_append(headerList, headerString.c_str());
	}
	curl_easy_setopt(this->curlHandle, CURLOPT_HTTPHEADER,
		headerList);

	// set basic auth if configured
	if (this->basicAuth.username.length() > 0) {
		std::string authString = std::string(this->basicAuth.username + ":" +
			this->basicAuth.password);
		curl_easy_setopt(this->curlHandle, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
		curl_easy_setopt(this->curlHandle, CURLOPT_USERPWD, authString.c_str());
	}
	/** set user agent */
	curl_easy_setopt(this->curlHandle, CURLOPT_USERAGENT,
		this->GetUserAgent().c_str());

	// set timeout
	if (this->timeout) {
		curl_easy_setopt(this->curlHandle, CURLOPT_TIMEOUT, this->timeout);
		// dont want to get a sig alarm on timeout
		curl_easy_setopt(this->curlHandle, CURLOPT_NOSIGNAL, 1);
	}
	// set follow redirect
	if (this->followRedirects == true) {
		curl_easy_setopt(this->curlHandle, CURLOPT_FOLLOWLOCATION, 1L);
	}
	// if provided, supply CA path
	if (!this->caInfoFilePath.empty()) {
		curl_easy_setopt(this->curlHandle, CURLOPT_CAINFO,
			this->caInfoFilePath.c_str());
	}
	else {
		curl_easy_setopt(this->curlHandle, CURLOPT_SSL_VERIFYPEER, 0);
		curl_easy_setopt(this->curlHandle, CURLOPT_SSL_VERIFYHOST, 0);
	}
	res = curl_easy_perform(this->curlHandle);
    ret.curl_code = res;
	if (res != CURLE_OK) {
		if (res == CURLE_OPERATION_TIMEDOUT) {
			ret.body = "Operation Timeout.";
		}
		else {
			ret.body = "Failed to query.";
		}
	}
	else {
		long long http_code = 0;
		curl_easy_getinfo(this->curlHandle, CURLINFO_RESPONSE_CODE, &http_code);
		ret.http_code = static_cast<int>(http_code);
	}
	fclose(fp);
	curl_easy_getinfo(this->curlHandle, CURLINFO_TOTAL_TIME,
		&this->lastRequest.totalTime);
	curl_easy_getinfo(this->curlHandle, CURLINFO_NAMELOOKUP_TIME,
		&this->lastRequest.nameLookupTime);
	curl_easy_getinfo(this->curlHandle, CURLINFO_CONNECT_TIME,
		&this->lastRequest.connectTime);
	curl_easy_getinfo(this->curlHandle, CURLINFO_APPCONNECT_TIME,
		&this->lastRequest.appConnectTime);
	curl_easy_getinfo(this->curlHandle, CURLINFO_PRETRANSFER_TIME,
		&this->lastRequest.preTransferTime);
	curl_easy_getinfo(this->curlHandle, CURLINFO_STARTTRANSFER_TIME,
		&this->lastRequest.startTransferTime);
	curl_easy_getinfo(this->curlHandle, CURLINFO_REDIRECT_TIME,
		&this->lastRequest.redirectTime);
	curl_easy_getinfo(this->curlHandle, CURLINFO_REDIRECT_COUNT,
		&this->lastRequest.redirectCount);
	// free header list
	curl_slist_free_all(headerList);
	// reset curl handle
	curl_easy_reset(this->curlHandle);
	return ret;
}

/**
 * @brief HTTP GET method
 *
 * @param url to query
 *
 * @return response struct
 */
RestClient::Response
RestClient::Connection::get(const std::string& url) {
  return this->performCurlRequest(url);
}
/**
 * @brief HTTP POST method
 *
 * @param url to query
 * @param data HTTP POST body
 *
 * @return response struct
 */
RestClient::Response
RestClient::Connection::post(const std::string& url,
                             const std::string& data) {
  /** Now specify we want to POST data */
  curl_easy_setopt(this->curlHandle, CURLOPT_POST, 1L);
  /** set post fields */
  curl_easy_setopt(this->curlHandle, CURLOPT_POSTFIELDS, data.c_str());
  curl_easy_setopt(this->curlHandle, CURLOPT_POSTFIELDSIZE, data.size());

  return this->performCurlRequest(url);
}
RestClient::Response RestClient::Connection::post(const std::string & uri, const char * data, size_t len)
{
	/** Now specify we want to POST data */
	curl_easy_setopt(this->curlHandle, CURLOPT_POST, 1L);
	/** set post fields */
	curl_easy_setopt(this->curlHandle, CURLOPT_POSTFIELDS, data);
	curl_easy_setopt(this->curlHandle, CURLOPT_POSTFIELDSIZE, len);

	return this->performCurlRequest(uri);
}
RestClient::Response RestClient::Connection::form(const std::string & uri, const std::map<std::string, std::string>& form, const std::string& file)
{
	/** Now specify we want to POST data */
	curl_easy_setopt(this->curlHandle, CURLOPT_POST, 1L);
	/** set post fields */
	struct curl_httppost *formpost = NULL;
	struct curl_httppost *lastptr = NULL;
	auto iter = form.begin();
	for (iter; iter != form.end(); ++iter) {
		curl_formadd(&formpost,
			&lastptr,
			CURLFORM_COPYNAME, iter->first.c_str(),
			CURLFORM_COPYCONTENTS, iter->second.c_str(),
			//CURLFORM_CONTENTTYPE, "text/plain",
			CURLFORM_END);
	}
	curl_formadd(&formpost,
		&lastptr,
		CURLFORM_COPYNAME, "logFile",
		CURLFORM_FILE, file.c_str(),
		CURLFORM_CONTENTTYPE, "application/x-gzip",
		CURLFORM_END);
	
	curl_easy_setopt(this->curlHandle, CURLOPT_HTTPPOST, formpost);
	auto resp = this->performCurlRequest(uri);

	curl_formfree(formpost);
	return resp;
}
/**
 * @brief HTTP PUT method
 *
 * @param url to query
 * @param data HTTP PUT body
 *
 * @return response struct
 */
RestClient::Response
RestClient::Connection::put(const std::string& url,
                            const std::string& data) {
  /** initialize upload object */
  RestClient::Helpers::UploadObject up_obj;
  up_obj.data = data.c_str();
  up_obj.length = data.size();

  /** Now specify we want to PUT data */
  curl_easy_setopt(this->curlHandle, CURLOPT_PUT, 1L);
  curl_easy_setopt(this->curlHandle, CURLOPT_UPLOAD, 1L);
  /** set read callback function */
  curl_easy_setopt(this->curlHandle, CURLOPT_READFUNCTION,
                   RestClient::Helpers::read_callback);
  /** set data object to pass to callback function */
  curl_easy_setopt(this->curlHandle, CURLOPT_READDATA, &up_obj);
  /** set data size */
  curl_easy_setopt(this->curlHandle, CURLOPT_INFILESIZE,
                     static_cast<int64_t>(up_obj.length));

  return this->performCurlRequest(url);
}
/**
 * @brief HTTP DELETE method
 *
 * @param url to query
 *
 * @return response struct
 */
RestClient::Response
RestClient::Connection::del(const std::string& url) {
  /** we want HTTP DELETE */
  const char* http_delete = "DELETE";

  /** set HTTP DELETE METHOD */
  curl_easy_setopt(this->curlHandle, CURLOPT_CUSTOMREQUEST, http_delete);

  return this->performCurlRequest(url);
}

RestClient::Response RestClient::Connection::upload(const std::string & uri, const std::string & file)
{
	/** initialize upload object */
	RestClient::Response resp;

    FILE *fd = nullptr;
    fopen_s(&fd, file.c_str(), "rb"); /* open file to upload */
	if (!fd) {
		resp.curl_code = -1;
		return resp;
	}
	/* to get the file size */
	fseek(fd, 0, SEEK_END);   // non-portable
	long size = ftell(fd);
	fseek(fd, 0, SEEK_SET);


	curl_easy_setopt(this->curlHandle, CURLOPT_UPLOAD, 1L);

	curl_easy_setopt(this->curlHandle, CURLOPT_READFUNCTION,
		RestClient::Helpers::upload_read_callback);

	/** set data object to pass to callback function */
	curl_easy_setopt(this->curlHandle, CURLOPT_READDATA, fd);
	/** set data size */
	curl_easy_setopt(this->curlHandle, CURLOPT_INFILESIZE,
		(curl_off_t)size);

	resp = this->performCurlRequest(uri);
	fclose(fd);
	return resp;

}

RestClient::Response RestClient::Connection::download(const std::string& url, const std::string& file, progress_cb cb)
{
	this->FollowRedirects(true);
	return this->performCurlRequest(url, file, cb);
}

