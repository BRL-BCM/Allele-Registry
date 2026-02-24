class AssistedHgvsController < ApplicationController
  unloadable

  layout 'registry_bootstrap_extensive_hdr_ftr'
  helper_method :convert_absolute_to_cdna

  before_filter  :find_project

  #def render_sequence
  #end
  def from_gene
  end
  def generate_hgvs
    logger.error('REGISTRY - generate_hgvs')
    logger.error(JSON.pretty_generate params)

    @local_id     = params["local"]
    @transcript   = params["transcript"]
    @current_gene = params["gene"]

    rackEnv = env
    #targetHost = GenboreeRegistry.last.gbHost 
    targetHost = GenboreeRegistry.find_by_project_id(@project.id).gbHost
    targetPath = "/refseq/#{@local_id}/sequence"

    @apiReq3 = GbApi::SimpleAsyncApiRequester.new(rackEnv, targetHost, @project)
    @apiReq3.nonGbApiTargetHost = true

    @response_back = String.new()

    @apiReq3.respCallback {|in_array|
       in_array[2].each { |chunk|
            @response_back << chunk
       }
    }

    @apiReq3.bodyFinish {
        status = 200
        headers = {}
        #response_hash = JSON.parse(@response_back)
        #logger.error  "Inside body finish"
        #logger.error  "#{response_hash}"
        #logger.error  "After printing response_hash"
        @apiReq3.renderToClient(self , :render_sequence)
    }

    @apiReq3.get(targetPath, "")

  end

  def show_transcripts

     @queried_gene = params['gene']
  
     logger.error('REGISTRY - inside show transcripts controller')
     logger.error(JSON.pretty_generate params)   

     rackEnv = env
     #targetHost = GenboreeRegistry.last.gbHost 
     targetHost = GenboreeRegistry.find_by_project_id(@project.id).gbHost
     # This needs to be changed when piotr gives new APIs
     targetPath = "/gene?HGNC.symbol=#{@queried_gene}"

     @apiReq = GbApi::SimpleAsyncApiRequester.new(rackEnv, targetHost, @project)
     @apiReq.nonGbApiTargetHost = true

     @response_from_registry = String.new()
     @transcripts_response = String.new()

     @apiReq.respCallback {|in_array|
        in_array[2].each { |chunk|
             @response_from_registry << chunk
        }
     }

     @apiReq.bodyFinish {
         status = 200
         headers = {}
         response_hash = JSON.parse(@response_from_registry)

         # Do error checking here for gene symbol - local identifier
         # If error then redirect to the view that shows error
         if(response_hash.keys.include?('errorType'))
           @apiReq.renderToClient(self ,:gene_symbol_resolution_failure)
         else
         # Else send second API request
             gene_uri = response_hash["@id"].split("/").last
             # Get identifier of the gene from previous request and 
             # initiate second call which will give you transcripts
             rackEnv = env
             #targetHost = GenboreeRegistry.last.gbHost 
	     targetHost = GenboreeRegistry.find_by_project_id(@project.id).gbHost
             # This needs to be changed when piotr gives new APIs
             targetPath = "/refseqs?gene=#{gene_uri}"
             @apiReq2 = GbApi::SimpleAsyncApiRequester.new(rackEnv, targetHost, @project)
             @apiReq2.nonGbApiTargetHost = true
             logger.error "REGISTY - did setup request"
             
             @apiReq2.respCallback {|in_array|
                logger.error "inside call back"
                in_array[2].each { |chunk|
                     @transcripts_response << chunk
                }
             }

             @apiReq2.bodyFinish {
                 logger.error "Inside body Finish"
                 status = 200
                 headers = {}
                 logger.error (@transcripts_response)
                 response_hash = JSON.parse(@transcripts_response)
                 logger.error (JSON.pretty_generate response_hash)
                 @apiReq.renderToClient(self ,:render_transcripts)
             }
             @apiReq2.get(targetPath, "")
          end
     }
     @apiReq.get(targetPath, "")
  end 


  def convert_absolute_to_cdna number, cds_start, cds_end
    if number <= cds_start
      return (number - cds_start - 1)
    end
    if number > cds_start and number <= cds_end
      return number - cds_start 
    end
    if number > cds_end
      return "*#{number - cds_end}"
    end
  end

end
