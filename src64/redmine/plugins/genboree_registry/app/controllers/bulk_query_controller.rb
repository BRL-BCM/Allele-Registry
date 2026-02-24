class BulkQueryController < ApplicationController
  unloadable

  include SingleAlleleHelper
  helper SingleAlleleHelper

  layout 'registry_bootstrap_extensive_hdr_ftr'

  before_filter  :find_project
  before_filter  :set_project_settings

  def start
  end
  def initiate

   rackEnv = env
   #targetHost = GenboreeRegistry.last.gbHost
   targetHost = GenboreeRegistry.find_by_project_id(@project.id).gbHost
   targetPath = "/alleles?file=hgvs"

   @apiReq = GbApi::SimpleAsyncApiRequester.new(rackEnv, targetHost, @project)

   @apiReq.nonGbApiTargetHost = true
   #@apiReq.headers = { 'Content-Type' => 'text/plain' }

   @response_from_registry = String.new()

   @apiReq.respCallback {|in_array|
      in_array[2].each { |chunk| 
           @response_from_registry << chunk
      }
   }

   @apiReq.bodyFinish {
       status = 200
       headers = {}
       @apiReq.renderToClient(self ,:initiate)
   }

   @apiReq.post(targetPath, {} , payload=params["hgvs"])

  end
end
