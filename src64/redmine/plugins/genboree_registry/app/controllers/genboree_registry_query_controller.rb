class GenboreeRegistryQueryController < ApplicationController
  unloadable

  layout 'registry_bootstrap_extensive_hdr_ftr'

  include SingleAlleleHelper
  helper SingleAlleleHelper

  before_filter  :find_project
  before_filter  :set_project_settings

  # This controller defines method that does not require login

  def landing
  end

  def single_allele
  end

  def vector_allele
  end

  # Overloaded action to handle both CAid/CNVid
  def by_canonicalid
    @canonicalId = params["canonicalid"]
    @controller = self
    if(@canonicalId =~ /^CNV/)
      targetHost = GenboreeRegistry.find_by_project_id(@project.id).cnvHost
      targetPath = "/allele/#{@canonicalId}"
    else
      targetHost = GenboreeRegistry.find_by_project_id(@project.id).gbHost
      targetPath = "/allele/#{@canonicalId}.json?fields=all+externalSources"
    end
    @apiReq = GbApi::SimpleAsyncApiRequester.new(env, targetHost, @project)
    @apiReq.nonGbApiTargetHost = true
    @response_from_registry = ""
    @status = nil
    @apiReq.respCallback {|in_array|
      #logger.info "Inside callback"
      @status = in_array[0]
      $stderr.debugPuts(__FILE__, __method__, 'STATUS', "Status from query: #{@status.inspect}")
      in_array[2].each { |chunk|
        @response_from_registry << chunk
      }
    }
    @apiReq.bodyFinish {
      if(@canonicalId =~ /^CNV/)
        @apiReq.renderToClient(@controller, :by_cnvid)
      else
        @apiReq.renderToClient(@controller, :single_allele)
      end
    }
    $stderr.debugPuts(__FILE__, __method__, 'DEBUG', "targetHost: #{targetHost}; targetPath: #{targetPath}")
    @apiReq.get(targetPath, "")
  end


  def by_caid
   ##$stderr.debugPuts(__FILE__, __method__, 'DEBUG', "++++++ Registry Interface: Inside by caid query")
   @caid = params["caid"]
   rackEnv = env
   #targetHost = GenboreeRegistry.last.gbHost #"localhost"
   targetHost = GenboreeRegistry.find_by_project_id(@project.id).gbHost
   targetPath = "/allele/#{@caid}.json?fields=all+externalSources"
   ##$stderr.debugPuts(__FILE__, __method__, 'DEBUG', "++++++ Registry Interface: Inside by caid query")
   @apiReq = GbApi::SimpleAsyncApiRequester.new(rackEnv, targetHost, @project)
   @apiReq.nonGbApiTargetHost = true
   @response_from_registry = String.new()
   #logger.info "Inspect apiReq #{@apiReq.inspect}"
   @apiReq.respCallback {|in_array|
      #logger.info "Inside callback"
      in_array[2].each { |chunk| 
           @response_from_registry << chunk
      }
   }
   @apiReq.bodyFinish {
       ##$stderr.debugPuts(__FILE__, __method__, 'DEBUG', "++++++ Registry Interface: Inside body finish #{@response_from_registry.inspect}")
       #logger.info "Inside bodyfinish"
       status = 200
       headers = {}
       #logger.error "Sending to single allele"
       @apiReq.renderToClient(self ,:single_allele)
   }
   @apiReq.get(targetPath, "")
    
  end

  def by_cnvid
    @cnvid = params["cnvid"]
    @controller = self
    #targetHost = GenboreeRegistry.last.gbHost #"localhost"
    targetHost = GenboreeRegistry.find_by_project_id(@project.id).cnvHost
    targetPath = "/allele/#{@cnvid}"
    @apiReq = GbApi::SimpleAsyncApiRequester.new(env, targetHost, @project)
    @apiReq.nonGbApiTargetHost = true
    @cnvResp = ""
    @status = nil
    @apiReq.respCallback {|in_array|
      #logger.info "Inside callback"
      @status = in_array[0]
      $stderr.debugPuts(__FILE__, __method__, 'STATUS', "Status from CNV query: #{@status.inspect}")
      in_array[2].each { |chunk|
        @cnvResp << chunk
      }
    }
    @apiReq.bodyFinish {
      @apiReq.renderToClient(@controller ,:by_cnvid)
    }
    @apiReq.get(targetPath, "")
  end

  def allele
     @caid = params_to_string params if(!params.key?("hgvsOrDescriptor"))
     #$stderr.debugPuts(__FILE__, __method__, "TIME", "DEBUG - escape bug #{@caid}")
     rackEnv = env
     targetHost = GenboreeRegistry.find_by_project_id(@project.id).gbHost
     fm = {}
     targetPath = ""
     @controller = self
     @isCNV = false
     if(!params.key?("hgvsOrDescriptor"))
        targetPath = "/allele.json?#{@caid}"
        @hgvsOrDescriptor = nil
     else
        @hgvsOrDescriptor = params["hgvsOrDescriptor"]
        # Figure out if it is hgvs or descriptor
        @isCNV = isCNVDescriptor?(@hgvsOrDescriptor)
        if(@isCNV)
          targetHost = GenboreeRegistry.find_by_project_id(@project.id).cnvHost
          targetPath = "/allele/cnv/desc/{desc}"
          fm = {:desc => @hgvsOrDescriptor}
        else
          targetPath = "/allele.json?hgvs={hgvs}"
          params["hgvs"] = @hgvsOrDescriptor
          fm[:hgvs] = @hgvsOrDescriptor
        end
     end
     @controller = self
     ##$stderr.debugPuts(__FILE__, __method__, "TIME", "DEBUG - escape bug #{targetPath}")
     @apiReq = GbApi::SimpleAsyncApiRequester.new(rackEnv, targetHost, @project)
     @apiReq.nonGbApiTargetHost = true
     @response_from_registry = String.new()
     @status = nil
     @apiReq.respCallback { |in_array|
        @status = in_array[0]
        in_array[2].each { |chunk|
          @response_from_registry << chunk
        }
     }
     @apiReq.bodyFinish {
       if(@isCNV)
         @apiReq.renderToClient(@controller ,:by_cnvid)
       else
         @apiReq.renderToClient(@controller, :single_allele)
       end
     }
     @apiReq.get(targetPath, fm)
  end

  # Action for handling any of the supported identifiers.
  def alleles 
     @caid = params_to_string params
     rackEnv = env
     @cnvReq = false
     @controller = self
     # Identifiers for Copy Number Variation (CNV) will need to be quried against the CNV registry
     if(params.key?("name") and params["name"] =~ /^CNV/)
       targetHost = GenboreeRegistry.find_by_project_id(@project.id).cnvHost
       targetPath = "/allele/#{params["name"]}"
       @cnvReq = true
     else # All other ids will be queried against the Allele Registry
       targetHost = GenboreeRegistry.find_by_project_id(@project.id).gbHost
       targetPath = "/alleles.json?#{@caid}&fields=all-genomicAlleles.coordinates-transcriptAlleles.coordinates"
     end
     @apiReq = GbApi::SimpleAsyncApiRequester.new(rackEnv, targetHost, @project)
     @apiReq.nonGbApiTargetHost = true
     @response_from_registry = String.new()
     @apiReq.respCallback {|in_array|
       @status = in_array[0]
        #$stderr.debugPuts(__FILE__, __method__, "TIME", "#{DateTime.now.strftime('%Q')}")
        in_array[2].each { |chunk|
             @response_from_registry << chunk
        }
     }
     @apiReq.bodyFinish {
       if(@cnvReq)
         @apiReq.renderToClient(@controller, :by_cnvid)
       else
         status = 200
         headers = {}
         if( JSON.parse(@response_from_registry).length == 1 )
           @caid = JSON.parse(@response_from_registry)[0]["@id"].split("/").last
           status = 302
           headers = { "location" => "by_caid?caid=#{@caid}" }
           @apiReq.sendToClient(status, headers, '')
         else
           @apiReq.renderToClient(@controller, :vector_allele)
         end
       end
     }
     @apiReq.get(targetPath, "")
  end

  private

  def find_project
    @project = Project.find(params[:id])
  rescue ActiveRecord::RecordNotFound
    render_404
  end

  def set_project_settings
    @project_settings = GenboreeRegistry.find_by_project_id(@project.id)
  end

end
