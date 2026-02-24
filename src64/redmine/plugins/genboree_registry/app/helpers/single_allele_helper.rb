module SingleAlleleHelper

  def find_project
    #$stderr.debugPuts(__FILE__, __method__, "TIME", "#{DateTime.now.strftime('%Q')}") 
    @project = Project.find(params[:id])
  rescue ActiveRecord::RecordNotFound
    render_404
  end

  def set_project_settings
    @project_settings = GenboreeRegistry.find_by_project_id(@project.id)
  end

  def isCNVDescriptor?(desc)
    retVal = false
    retVal = desc.match( /^(?:\S+)\s+(?:\S+)$/ )
    if( retVal )
      locStr = desc.split(" ")[1]
      retVal = locStr.match( /^(?:(?:[^()\-]+)(?:-(?:[^()-]+))?)?\((?:[^:]+):(?:[^)]+)\)x(?:\d+)(?:,(?:\d+))?$/ )
      if( !retVal )
        retVal = locStr.match( /^(?:(?:[^()\-]+)(?:-(?:[^()-]+))?)?\((?:[^:]+):(?:(?:\d+_\d+)|(?:\([?_\-0-9()]+\)))\)x(?:\d+)(?:,(?:\d+))?$/ )
        if( !retVal )
          retVal = locStr.match( /^(?:[^:]+):(?:(?:\d+_\d+)|(?:\([?_\-0-9()]+\)))x(?:\d+)(?:,(?:\d+))?$/ )
        end
      end
    end
    return ( retVal ? true : false)
  end

  def params_to_string in_params
   #$stderr.debugPuts(__FILE__, __method__, "TIME", "#{DateTime.now.strftime('%Q')}") 
   caid = String.new()
   params.each do |key,value|
     if ! ["action", "controller","id"].include?(key)
       #$stderr.debugPuts(__FILE__, __method__, "TIME", "DEBUG - escape bug #{caid}") 
       caid << "&#{key}=#{CGI::escape(value)}"
       #$stderr.debugPuts(__FILE__, __method__, "TIME", "DEBUG - escape bug #{caid}") 
     end
   end
   #$stderr.debugPuts(__FILE__, __method__, "TIME", "#{DateTime.now.strftime('%Q')}") 
   #$stderr.debugPuts(__FILE__, __method__, "TIME", "DEBUG - escape bug #{caid}") 
   caid = caid.sub(/^&/,"")
   #$stderr.debugPuts(__FILE__, __method__, "TIME", "DEBUG - escape bug #{caid}") 
   return caid
  end

  def is_registered? allele_json
    #$stderr.debugPuts(__FILE__, __method__, "TIME", "#{DateTime.now.strftime('%Q')}") 
    if allele_json["@id"] == "_:CA"
      return false
    elsif allele_json["@id"] == "_:PA"
      return false
    else
      return true 
      #( ! (allele_json["@id"] == "_:CA") )
    end
  end

  def is_aa_allele_registered? allele_json
    return ( ! (allele_json["@id"] == "_:PA") )
  end

  def uri_to_caid in_string
    if in_string == "_:CA"
       return "Get CA identifier"
    elsif in_string == "_:PA"
       return "Get Allele identifier"
    elsif in_string.match(/CA[0-9]+/)
       return in_string.match(/CA[0-9]+/)[0]
    elsif in_string.match(/PA[0-9]+/)
       return in_string.match(/PA[0-9]+/)[0] 
    else
       return ""
    end
  end

  def has_genomic_alleles? allele_json
    #$stderr.debugPuts(__FILE__, __method__, "TIME", "#{DateTime.now.strftime('%Q')}") 
      return allele_json.keys.include?("genomicAlleles")
  end

  def has_transcript_alleles? allele_json
    #$stderr.debugPuts(__FILE__, __method__, "TIME", "#{DateTime.now.strftime('%Q')}") 
      return allele_json.keys.include?("transcriptAlleles")
  end

  def has_external_records? allele_json
    #$stderr.debugPuts(__FILE__, __method__, "TIME", "#{DateTime.now.strftime('%Q')}") 
      return allele_json.keys.include?("externalRecords")
  end

  def has_aminoacid_alleles? allele_json
    #$stderr.debugPuts(__FILE__, __method__, "TIME", "#{DateTime.now.strftime('%Q')}") 
      return allele_json.keys.include?("aminoAcidAlleles")
  end

  def get_genomic_hgvs allele_json
    #$stderr.debugPuts(__FILE__, __method__, "TIME", "#{DateTime.now.strftime('%Q')}") 
      #logger.error "Inside private method get_genomic_hgvs"
      # This will become available to view upon return
      genomic_hgvs = Array.new()
      # if Genomic allele
      if allele_json.keys.include?("genomicAlleles")
         # Iterate through each genomic allele
         allele_json["genomicAlleles"].each do |allele|
           hgvs_expressions = Array.new()
           assembly = ""
           # Iterate through each HGVS expression
           allele["hgvs"].each do |expression|
                if expression.include?("NC_")
                 assembly = allele["referenceGenome"]
                end
                hgvs_expressions.push(expression)
           end # end expression iteration
           genomic_hgvs.push({"expressions" => hgvs_expressions, "assembly" => assembly})
         end # end of all genomic alleles
      end # end of if statement
      return genomic_hgvs
  end
 
  def get_transcript_hgvs allele_json
    #$stderr.debugPuts(__FILE__, __method__, "TIME", "#{DateTime.now.strftime('%Q')}") 
      #logger.error "Inside private method get_transcript_hgvs"
      transcript_hgvs = Array.new()

      if allele_json.keys.include?("transcriptAlleles")
	allele_json["transcriptAlleles"].each do |allele|

	  expressions = Array.new()
	  aa_variation = String.new()
	  aa_well_defined = String.new()
	  gene_symbol = String.new()

	  if allele.keys.include?("proteinEffect")
	    if(allele["proteinEffect"]["hgvs"])
	      aa_variation = allele["proteinEffect"]["hgvs"]
	    end
	    if(allele["proteinEffect"]["hgvsWellDefined"])
	      aa_well_defined = allele["proteinEffect"]["hgvsWellDefined"]
	    end
	  end

	  if(allele["geneSymbol"])
	    gene_symbol = allele["geneSymbol"]
	  end

	  allele["hgvs"].each do |expression|
	    expressions.push(expression)
	  end

	  transcript_hgvs.push({"expressions" => expressions,
			 "aa_variation" => aa_variation,
			 "aa_well_defined" => aa_well_defined,
	                 "gene_symbol" => gene_symbol })
	end
      end
      return transcript_hgvs
  end

  def get_aminoacid_hgvs allele_json
    #$stderr.debugPuts(__FILE__, __method__, "TIME", "#{DateTime.now.strftime('%Q')}") 
    #logger.error "Inside private method get_amino_hgvs"
    aminoacid_hgvs = Array.new()

    if allele_json.keys.include?("aminoAcidAlleles")
      allele_json["aminoAcidAlleles"].each do |allele|
        transcript_variation = Array.new()
        gene_symbol = String.new()
        expressions = Array.new()

	allele["hgvs"].each do |expression|
	  expressions.push(expression)
	end

        if allele.keys.include?("hgvsMatchingTranscriptVariant")
          if(allele["hgvsMatchingTranscriptVariant"])
	    allele["hgvsMatchingTranscriptVariant"].each do |tvar|
              transcript_variation.push(tvar)
	    end
          end
        end

	if(allele["geneSymbol"])
	    gene_symbol = allele["geneSymbol"]
	end

	aminoacid_hgvs.push({"expressions" => expressions,
			     "transcript_variation" => transcript_variation,
	                     "gene_symbol" => gene_symbol })
      end
    end

    return aminoacid_hgvs
  end

  def get_clinvar_variations allele_json
    #$stderr.debugPuts(__FILE__, __method__, "TIME", "#{DateTime.now.strftime('%Q')}") 
      clinvar_array = Array.new()
      if has_external_records? allele_json
        if allele_json["externalRecords"].key?("ClinVarVariations")
           allele_json["externalRecords"]["ClinVarVariations"].each do |variation|
             clinvar_array.push(
                 {
                   "vid" => variation["variationId"],
                   "link" => variation["@id"],
                   "rcv" => (variation.key?("RCV")) ? (variation["RCV"].join(",")) : ('')
                 }
             )
           end
        end
      end
      return clinvar_array
  end

  def get_cosmic_variations allele_json
    cosmic_array = Array.new()
    if has_external_records? allele_json
      if allele_json["externalRecords"].key?("COSMIC")
        allele_json["externalRecords"]["COSMIC"].each do |variation|
	  if variation["active"] == true
               cosmic_array.push({
	         "vid" => variation["id"],
	         "link" => variation["@id"]
	       })
	  end
	end
      end
    end
    return cosmic_array
  end

  def get_gnomad allele_json
    #$stderr.debugPuts(__FILE__, __method__, "TIME", "#{DateTime.now.strftime('%Q')}") 
      gnomad_array = Array.new()
      if has_external_records? allele_json
        if allele_json["externalRecords"].keys.include?("gnomAD")
           allele_json["externalRecords"]["gnomAD"].each do |variation|
             gnomad_array.push({  
                  "link" => variation["@id"],
                  "id" => variation["variant"]
              })
           end
        end
      end
      #logger.error("REGISTRY - Printing gnomAD array #{gnomad_array}")
      return gnomad_array
  end

  def get_myvariant allele_json
    #$stderr.debugPuts(__FILE__, __method__, "TIME", "#{DateTime.now.strftime('%Q')}") 
      myvariant_array = Array.new()
      if has_external_records? allele_json
          if allele_json["externalRecords"].keys.include?("MyVariantInfo_hg19")
        allele_json["externalRecords"]["MyVariantInfo_hg19"].each do |variation|
            myvariant_array.push({"id" => "#{variation["id"]} (hg19)", "link" => "#{variation["@id"]}" })
        end
          end
          if allele_json["externalRecords"].keys.include?("MyVariantInfo_hg38")
        allele_json["externalRecords"]["MyVariantInfo_hg38"].each do |variation|
            myvariant_array.push({"id" => "#{variation["id"]} (hg38)", "link" => "#{variation["@id"]}" })
        end
          end
      end
      return myvariant_array
  end

  def get_dbsnp allele_json
    #$stderr.debugPuts(__FILE__, __method__, "TIME", "#{DateTime.now.strftime('%Q')}") 
      dbsnp_array = Array.new()
      if has_external_records? allele_json
        if allele_json["externalRecords"].keys.include?("dbSNP")
           allele_json["externalRecords"]["dbSNP"].each do |variation|
           dbsnp_array.push({
                 "rsid" => variation["rs"] ,
                 "link" => variation["@id"]
           })
           end
        end
      end
      return dbsnp_array
  end

  def get_exac allele_json
    #$stderr.debugPuts(__FILE__, __method__, "TIME", "#{DateTime.now.strftime('%Q')}") 
      exac_array = Array.new()
      if has_external_records? allele_json
        if allele_json["externalRecords"].keys.include?("ExAC")
           allele_json["externalRecords"]["ExAC"].each do |variation|
             exac_array.push({  
                  "link" => variation["@id"],
                  "id" => variation["variant"]
              })
           end
        end
      end
      #logger.error("REGISTRY - Printing ExAC array #{exac_array}")
      return exac_array
  end

  def get_allelic_epigenome allele_json
    allelic_epigenome = Array.new()
    if has_external_records? allele_json
      if allele_json["externalRecords"].keys.include?("AllelicEpigenome")
	allele_json["externalRecords"]["AllelicEpigenome"].each do |variation|
	  allelic_epigenome.push({
             "link" => variation["@id"],
             "id" => "Allelic Epigenome"
	  })
	end
      end
    end
    return allelic_epigenome
  end

  def get_gene allele_json
    #$stderr.debugPuts(__FILE__, __method__, "TIME", "#{DateTime.now.strftime('%Q')}") 

      #logger.error("REGISTRY - Inside get_gene method")

      gene_and_ncbi_array = Array.new()

      if has_transcript_alleles? allele_json
         allele_json["transcriptAlleles"].each do |allele|
           if allele.keys.include?("geneSymbol") and allele.keys.include?("geneNCBI_id")
         if ! allele["geneSymbol"].nil? 
           if ! allele["geneNCBI_id"].nil? 
         gene_and_ncbi_array.push( {"geneSymbol" => allele["geneSymbol"] ,
                                       "geneNCBI_id" => allele["geneNCBI_id"]} )
           end
         end
           end
         end
      elsif has_aminoacid_alleles? allele_json
         allele_json["aminoAcidAlleles"].each do |allele|
           if allele.keys.include?("geneSymbol") and allele.keys.include?("geneNCBI_id")
         if ! allele["geneSymbol"].nil? 
           if ! allele["geneNCBI_id"].nil? 
           gene_and_ncbi_array.push( {"geneSymbol" => allele["geneSymbol"] ,
                                       "geneNCBI_id" => allele["geneNCBI_id"]} )
           end
         end
           end
	 end
      end

      uniq_gene_and_ncbi_array = gene_and_ncbi_array.uniq

      gene_response = Array.new()
      
      uniq_gene_and_ncbi_array.each do |gene|
         gene_response.push({
           "externalRecords" => {
            "NCBI" => { 
              "id" => gene["geneNCBI_id"],
              "@id" => "http://www.ncbi.nlm.nih.gov/gene/#{gene["geneNCBI_id"]}"
            },
            "HGNC" => {
              "symbol" => gene["geneSymbol"],
              "id" => gene["geneSymbol"],
              "@id" => "http://www.genenames.org/cgi-bin/gene_symbol_report?match="
            }
           }
         })
      end

      #logger.error("REGISTRY - Parsing the uniq gene array")
      #logger.error("REGISTRY - #{uniq_gene_array}")

      # uniq_gene_array.each do |gene|
      #     f = open(URI.parse(gene)) rescue nil
      #     #logger.error("REGISTRY - #{f}")
      #     if ! f.nil?
      #       gene_response.push(JSON.parse(f.read))
      #     end
      # end

      #logger.error("REGISTRY - After sending requests")
      #logger.error("REGISTRY - #{JSON.pretty_generate gene_response}")

      return gene_response 
  end

  def parse_gene_links gene_json
    #$stderr.debugPuts(__FILE__, __method__, "TIME", "#{DateTime.now.strftime('%Q')}") 

      #logger.error("REGISTRY - Inside parse_gene_links")
      #logger.error("REGISTRY - printing gene_json #{gene_json}")

      gene_array = Array.new()

      gene_json.each do |gene|
        if gene.keys.include?("externalRecords")
           this_hash = Hash.new()
             if gene["externalRecords"].keys.include?("HGNC")
              this_hash["symbol"]    = gene["externalRecords"]["HGNC"]["symbol"]
              this_hash["hgnc"]      = gene["externalRecords"]["HGNC"]["@id"],
              this_hash["hgnc_id"]   = gene["externalRecords"]["HGNC"]["id"]
             end
             if gene["externalRecords"].keys.include?("NCBI")
              this_hash["ncbi_id"]      = gene["externalRecords"]["NCBI"]["id"]
              this_hash["ncbi"]         = gene["externalRecords"]["NCBI"]["@id"]
             end
             if gene["externalRecords"].keys.include?("MANEPrefRefSeq")
              this_hash["preferredRefSeq"] = gene["externalRecords"]["MANEPrefRefSeq"]["id"]
             end
           gene_array.push(this_hash)
        end
     end
     
     return gene_array
  end

  def find_preferred_transcripts in_array
    # This function makes unfortunate sync call.
    # Generates Gene => Preferred Transcript mapping which can be used to show information in the single allele view.
    out_hash = Hash.new
    in_array.each do |g|
      res = JSON.parse(Net::HTTP.get(GenboreeRegistry.find_by_project_id(@project.id).gbHost, "/genes?name=#{g["symbol"]}"))
      if ! res.empty?
        # $stderr.debugPuts(__FILE__, __method__, "MANE", "#{res.inspect}") 
        out_hash[g["symbol"]] = res.select{|j| j["externalRecords"]["HGNC"]["symbol"] == g["symbol"]}.first["externalRecords"]["MANEPrefRefSeq"]["id"] rescue nil
      end
    end    
    # $stderr.debugPuts(__FILE__, __method__, "MANE", "#{out_hash.inspect}")     
    return out_hash
  end

  def get_grch38_chr allele_json
    #$stderr.debugPuts(__FILE__, __method__, "TIME", "#{DateTime.now.strftime('%Q')}") 
      chr = "-"

      if allele_json.keys.include?("genomicAlleles")
         # Iterate through each genomic allele
         allele_json["genomicAlleles"].each do |allele|
            if allele["referenceGenome"] == "GRCh38"
              chr = allele["chromosome"]
            end 
         end
      end

      return chr
  end

  def get_grch38_reference_sequence_start_end allele_json
      to_send_back = Hash.new()

      if allele_json.keys.include?("genomicAlleles")
         allele_json["genomicAlleles"].each do |allele|
            if allele["referenceGenome"] == "GRCh38"
              #chr = allele["referenceSequence"]
          to_send_back["g38_start"]  = allele["coordinates"][0]["start"]
          to_send_back["g38_end"]    = allele["coordinates"][0]["end"]
          #to_send_back["g38_refseq"] = allele["hgvs"].select{|i| i.include?("NC_")}[0].split(":")[0] rescue allele["hgvs"].select{|i| i.include?("NT_")}[0].split(":")[0]
          to_send_back["g38_refseq"] = allele["hgvs"].select{|i| i.include?("NC_") or i.include?("NT_") or i.include?("NW_") }[0].split(":")[0] 
          #to_send_back["g38_refseq"] = to_send_back["g38_refseq"].to_s.split(":")[0]
            end 
         end
      end

      return to_send_back
  end

  def get_uniq_cp allele_json
    #$stderr.debugPuts(__FILE__, __method__, "TIME", "#{DateTime.now.strftime('%Q')}") 
      cp = Array.new()
      if allele_json.keys.include?("transcriptAlleles")
         allele_json["transcriptAlleles"].each do |allele|
           chgvs = ""
           phgvs = ""
           if allele.keys.include?('hgvs')
             chgvs = allele["hgvs"][0].split(":")[1]
           end
	   if allele.keys.include?('proteinEffect')
	     if allele["proteinEffect"]["hgvs"]
	       phgvs = allele["proteinEffect"]["hgvs"].split(":")[1]
	     end
	   end
           cp.push("#{chgvs}$#{phgvs}")
         end
      end
      return cp
  end

  def get_unique_gene_symbols allele_json

    find_project
    set_project_settings

    gene_symbols = Array.new

    if allele_json.keys.include?("transcriptAlleles")
      allele_json["transcriptAlleles"].each do |allele|
        if ! allele["geneSymbol"].nil?
          gene_symbols.push(allele["geneSymbol"])
          #$stderr.debugPuts(__FILE__, __method__, "DEBUG", "#{gene_symbols.inspect}") 
        end
      end
    end

    if allele_json.keys.include?("aminoAcidAlleles")
      allele_json["aminoAcidAlleles"].each do |allele|
	if ! allele["geneSymbol"].nil?
          gene_symbols.push(allele["geneSymbol"])
	end
      end
    end

    uniq_gene_symbols = gene_symbols.uniq
    return uniq_gene_symbols
  end

  # Helper methods added when externalSources are introduced in the Registry
  def has_user_contributed_sources allele_json
    if allele_json["externalSources"]
      return true
    else
      return false
    end
  end

  #def initialize_external_sources
  #  gbHost = GenboreeRegistry.find_by_project_id(@project.id).gbHost
  #  response = Net::HTTP.get(URI.parse("http://#{gbHost}/externalSources"))
  #  return JSON.parse(response)
  #end


    def generate_table_body(hash_response)
        table = ""
        hash_response.each do |hit|
            caid = uri_to_caid(hit["@id"])
            genomic_alleles = hit["genomicAlleles"]
            transcript_alleles = hit["transcriptAlleles"]
            aminoacid_alleles = hit["aminoAcidAlleles"]
            hgvs_trans = ''
            hgvs_amino = ''
	    hgvs_trans_and_amino = ''
            if transcript_alleles
                d = transcript_alleles.map{|x| [x['hgvs'][0].split(':')[1], x['proteinEffect'] ? (x['proteinEffect']['hgvs'] ? x['proteinEffect']['hgvs'].split(':')[1] : '') : '']}.uniq
                hgvs_trans = d.map{|x| "#{ERB::Util.html_escape(x[0])}<br>"}.join('')
                hgvs_amino = d.map{|x| "#{ERB::Util.html_escape(x[1])}<br>"}.join('')
		hgvs_trans_and_amino = d.map{|x| x[1] == '' ? "#{ERB::Util.html_escape("#{x[0]}")}<br>" : "#{ERB::Util.html_escape("#{x[0]} (#{x[1]})")}<br>"}.join('')
            elsif aminoacid_alleles
                hgvs_amino = ERB::Util.html_escape(aminoacid_alleles[0]["hgvs"][0].split(':')[1]) + '<br>'
            end
            table << '<tr class="body-row"><td>'
            table << genomic_alleles.find{|ga| ga["referenceGenome"] == "GRCh38"}['chromosome'].to_s if genomic_alleles
            table << '</td><td class="grch38-gdot">'
            if genomic_alleles
                table << ERB::Util.html_escape(genomic_alleles.find{|ga| ga["referenceGenome"] == "GRCh38"}["hgvs"][0].split(":")[1])
            end
            table << '</td><td class="canonical-allele">'
	    table << '<a href="/redmine/projects/registry/genboree_registry/by_caid?caid='
	    table << caid
	    table << '"><span>'
            table << caid
            table << '</span></a>'
            table << '<span style="font-size: 0.7em">[Amino-Acid Allele]</span>' if aminoacid_alleles
            table << '</td><td class="gene-symbols">'
            if transcript_alleles
                table << transcript_alleles.map{|x| x['geneSymbol']}.compact.uniq.sort.join(",")
            end
            #table << '</td><td class="transcript-cdot">'
            #table << hgvs_trans
            #table << '</td><td class="aa-pdot">'
            #table << hgvs_amino
            table << '</td><td class="transcript-aa-cdot-pdot transcript-cdot aa-pdot">'
	    table << hgvs_trans_and_amino

            table << '</td><td class="linkout-column">'
            table << '<a data-toggle="tooltip" title="See this allele in single allele viewer" href="/redmine/projects/registry/genboree_registry/by_caid?caid=' << caid << '"><span class="label label-jsonld"><i class="fa fa-info" aria-hidden="true"></i></span></a>'
            table << ' <a data-toggle="tooltip" title="JSON document (for developers)" href="/allele/' << caid  << '"><span class="label label-jsonld"><i class="fa fa-file-text-o" aria-hidden="true"></i></span></a>'
            table << ' <a href="' << @project_settings.calculatorPath << caid << '" data-toggle="tooltip" title="Interpret this variant using pathogenicity calculator"' << '><span class="label label-jsonld"><i class="fa fa-calculator" aria-hidden="true"></i></span></a>'
            external_records = hit["externalRecords"]
            if external_records
                if external_records['ClinVarVariations']
                    external_records['ClinVarVariations'].each do |var|
                        table << ' <a data-toggle="tooltip" title="View this allele in the ClinVar" href=' << var['@id'] << '><span class="label label-jsonld label-clinvar">ClinVar</span></a>'
                    end
                end
                if external_records['dbSNP']
                    external_records['dbSNP'].each do |var|
                        table << ' <a data-toggle="tooltip" title="View this allele in the dbSNP" href=' << var['@id'] << '><span class="label label-jsonld label-dbsnp">dbSNP</span></a>'
                    end
                end
                if external_records['ExAC']
                    external_records['ExAC'].each do |var|
                        table << ' <a data-toggle="tooltip" title="View this allele in the ExAC browser" href=' << var['@id'] << '><span class="label label-jsonld label-exac">ExAC</span></a>'
                    end
                end
                if external_records['gnomAD']
                    external_records['gnomAD'].each do |var|
                        table << ' <a data-toggle="tooltip" title="View this allele in the gnomAD browser" href=' << var['@id'] << '><span class="label label-jsonld label-gnomad">gnomAD</span></a>'
                    end
                end
                if external_records['COSMIC']
                    external_records['COSMIC'].each do |var|
		     if var['active'] == true
                        table << ' <a data-toggle="tooltip" title="View this allele in the COSMIC browser" href=' << var['@id'] << '><span class="label label-jsonld label-cosmic">COSMIC</span></a>'
		     end
                    end
                end
                if external_records['AllelicEpigenome']
                    external_records['AllelicEpigenome'].each do |var|
                        table << ' <a data-toggle="tooltip" title="Allelic imblance data from Roadmap epigenomics" href=' << var['@id'] << '><span class="label label-jsonld label-allelic">Allelic</span></a>'
                    end
                end
            end
            table << "</td></tr>\n"
        end
        return table
    end

end
