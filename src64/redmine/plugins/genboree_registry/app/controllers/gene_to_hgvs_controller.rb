class GeneToHgvsController < ApplicationController
  unloadable

  include GeneToHgvsHelper
  helper GeneToHgvsHelper

  include SingleAlleleHelper
  helper SingleAlleleHelper

  before_filter  :find_project
  before_filter  :set_project_settings

  layout 'registry_bootstrap_extensive_hdr_ftr'
  def new
    @gene_query = {
      "gene" => "",
      "type" => "c.",
      "position" => 0,
      "reference" => "A",
      "alternate" => "T"
    }
  end
  def create
   #$stderr.debugPuts(__FILE__, __method__, 'DEBUG', "++++++ FUZZY QUERY: parameters #{params.inspect}")
   @gene_query = {
      "gene" => params[:gene],
      "type" => params[:type],
      "position" => params[:position],
      "reference" => params[:reference],
      "alternate" => params[:alternate] 
   }
   gene = @gene_query["gene"]
   variation = join_to_get_cdot_pdot @gene_query["type"], @gene_query["position"], @gene_query["reference"], @gene_query["alternate"]
   type = @gene_query["type"]
   query_and_render gene, variation, type
  end

  # new object for indel query
  def indel_new
    @gene_query = {
      "gene" => "",
      "type" => "c.",
      "variation" => ""
    }
  end

  def indel_create
    gene = "#{params[:gene]}"
    variation = "#{params[:type]}#{params[:variation]}"
    type = "#{params[:type]}"
    query_and_render gene, variation, type
  end

  def query_and_render gene, variation, type
   @searched_gene = gene
   @searched_variation = variation
   @searched_type = type

   rackEnv = env
   # targetHost = GenboreeRegistry.last.gbHost
   targetHost = GenboreeRegistry.find_by_project_id(@project.id).gbHost
   # Query 1 for reference sequences
   #targetPath = "/refseqs?gene=#{params[:gene]}&fields=all-@context-@id-gene"
   #gene = params[:gene]
   #variation = join_to_get_cdot_pdot @gene_query["type"], @gene_query["position"], @gene_query["reference"], @gene_query["alternate"]
   # This entire thing should be a new function which will take gene and variation as input
   targetPath = "/matchAlleles?gene=#{@searched_gene}&variant=#{@searched_variation}"
   @apiReq = GbApi::SimpleAsyncApiRequester.new(rackEnv, targetHost, @project)
   @apiReq.nonGbApiTargetHost = true
   @response_from_registry = String.new()
   @apiReq.respCallback {|in_array|
      in_array[2].each { |chunk| 
           @response_from_registry << chunk
      }
   }
   @apiReq.bodyFinish {
       status = 200
       headers = {}
       if JSON.parse(@response_from_registry)
          hash_response = JSON.parse(@response_from_registry)
	  # Parse nucleotide alleles
          @hgvs = []
          @response_from_registry = []
          if(hash_response["canonicalAlleles"])
            hash_response["canonicalAlleles"].each do |canonical_allele|
              @hgvs << canonical_allele["hgvs"]
              @response_from_registry << canonical_allele["result"]
            end
            #$stderr.debugPuts(__FILE__, __method__, 'DEBUG', "++++++ FUZZY QUERY: hgvs array: #{@hgvs},  #{@response_from_registry.inspect}")
          end
          @response_from_registry = @response_from_registry.to_json
          @hgvs = @hgvs.join("\n")
	  # Parse amino-acid alleles
          @hgvs_p = []
          @response_from_registry_protein = []
          if(hash_response["proteinAlleles"])
            hash_response["proteinAlleles"].each do |protein_allele|
              @hgvs_p << protein_allele["hgvs"]
              @response_from_registry_protein << protein_allele["result"]
            end
          end
          @response_from_registry_protein = @response_from_registry_protein.to_json
          @hgvs_p = @hgvs_p.join("\n")
          #$stderr.debugPuts(__FILE__, __method__, 'DEBUG', "++++++ FUZZY QUERY: hgvs array: #{@hgvs_p.inspect},  #{@response_from_registry_protein.inspect}")
	  # Error returned
	  @error_object = {}
	  if(hash_response["description"])
	    @error_object["description"] = hash_response["description"]
	    @error_object["message"] = hash_response["message"]
	  end
          @apiReq.renderToClient(self,:fuzzy_query)
       else
         @apiReq.renderToClient(self,:suggestions)
       end
   }
   @apiReq.get(targetPath, {})
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
