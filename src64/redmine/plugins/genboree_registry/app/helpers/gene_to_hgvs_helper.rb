module GeneToHgvsHelper
  ## def suggest_hgvs json_in, type, position, reference, alternate
  ##   response = JSON.parse(json_in)
  ##   if type == "c."
  ##     return response.select{|a| a["type"] == "transcript"}.map{|b| "#{b["externalRecords"]["NCBI"]["id"]}:c.#{position}#{reference}>#{alternate}"}
  ##   end
  ##   if type == "p."
  ##     return response.select{|a| a["type"] == "gene"}.map{|b| "#{b["externalRecords"]["NCBI"]["id"]}:p.#{reference}#{position}#{alternate}"}
  ##   end
  ## end

  def join_to_get_cdot_pdot type, position, reference, alternate
    if type == "c."
      return "c.#{position}#{reference}>#{alternate}"
    end
    if type == "p."
      return "p.#{reference}#{position}#{alternate}"
    end
  end
end
