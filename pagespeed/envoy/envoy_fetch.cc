/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 * 
 *   http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "envoy_fetch.h"

#include <algorithm>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <set>

#include "net/instaweb/http/public/async_fetch.h"
#include "net/instaweb/http/public/inflating_fetch.h"
#include "pagespeed/kernel/base/basictypes.h"
#include "pagespeed/kernel/base/condvar.h"
#include "pagespeed/kernel/base/message_handler.h"
#include "pagespeed/kernel/base/pool.h"
#include "pagespeed/kernel/base/pool_element.h"
#include "pagespeed/kernel/base/statistics.h"
#include "pagespeed/kernel/base/string_util.h"
#include "pagespeed/kernel/base/thread_system.h"
#include "pagespeed/kernel/base/timer.h"
#include "pagespeed/kernel/base/writer.h"
#include "pagespeed/kernel/http/request_headers.h"
#include "pagespeed/kernel/http/response_headers.h"
#include "pagespeed/kernel/http/response_headers_parser.h"
#include "pagespeed_remote_data_fetcher.h"

namespace net_instaweb {

// Default keepalive 60s.
const int64 keepalive_timeout_ms = 60000;

PagespeedDataFetcherCallback::PagespeedDataFetcherCallback(EnvoyFetch* fetch) { 
  fetch_ = fetch;
}

void PagespeedDataFetcherCallback::onSuccess(Envoy::Http::MessagePtr& response) {
  std::cout << "PagespeedDataFetcherCallback::onSuccess data:" << response->bodyAsString() << "\n";
  std::cout.flush();

  fetch_->setResponse(response->headers(), response->body());

  // set response body
  // fetch_->async_fetch_->Write(StringPiece(response->body()->toString(), response->body()->length()),
  //                             fetch_->message_handler());
}

void PagespeedDataFetcherCallback::onFailure(FailureReason reason) {
  std::cout << "PagespeedDataFetcherCallback::onFailure\n";
  std::cout.flush();
}

EnvoyFetch::EnvoyFetch(const GoogleString& url,
                   AsyncFetch* async_fetch,
                   MessageHandler* message_handler,
                   EnvoyClusterManager* cluster_manager)
    : str_url_(url),
      fetcher_(NULL),
      async_fetch_(async_fetch),
      // TODO : Replace parser_ initialization
      // parser_(async_fetch->response_headers()),
      parser_(nullptr),
      message_handler_(message_handler),
      cluster_manager_(cluster_manager),
      done_(false),
      content_length_(-1),
      content_length_known_(false) {

}

EnvoyFetch::~EnvoyFetch() {
  delete cb;
}

void EnvoyFetch::FetchWithEnvoy() {
  envoy::api::v2::core::HttpUri http_uri;
  http_uri.set_uri("http://localhost:80");
  http_uri.set_cluster("cluster1");
  std::string uriHash("123456789");

  cb = new PagespeedDataFetcherCallback(this);
  std::unique_ptr<PagespeedRemoteDataFetcher> PagespeedRemoteDataFetcherPtr = std::make_unique<PagespeedRemoteDataFetcher>(
    cluster_manager_->getClusterManager(), http_uri, uriHash, *cb);

  PagespeedRemoteDataFetcherPtr->fetch();
  cluster_manager_->getDispatcher()->run(Envoy::Event::Dispatcher::RunType::Block);
  delete this;
}


// This function is called by EnvoyUrlAsyncFetcher::StartFetch.
void EnvoyFetch::Start() {
  std::function<void()> fetch_fun_ptr =
      std::bind(&EnvoyFetch::FetchWithEnvoy, this);
  cluster_manager_->getDispatcher()->post(fetch_fun_ptr);
  cluster_manager_->getDispatcher()->run(Envoy::Event::Dispatcher::RunType::NonBlock);
}


bool EnvoyFetch::Init() {
  return true;
}


// This function should be called only once. The only argument is sucess or
// not.
void EnvoyFetch::CallbackDone(bool success) {
}

void EnvoyFetch::setResponse(Envoy::Http::HeaderMap& headers, Envoy::Buffer::InstancePtr& body) {
  // if (async_fetch_->response_headers()->Has(HttpAttributes::kXOriginalContentLength)) {
  //     async_fetch_->extra_response_headers()->SetOriginalContentLength(body->length());
  // }
}

MessageHandler* EnvoyFetch::message_handler() {
  return message_handler_;
}

void EnvoyFetch::FixUserAgent() {
}

// Prepare the request data for this fetch, and hook the write event.
int EnvoyFetch::InitRequest() {
  return 0;
}

int EnvoyFetch::Connect() {
  return 0;
}

}  // namespace net_instaweb