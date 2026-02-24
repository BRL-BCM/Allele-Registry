=begin
Copyright (c) 2001-2016 Baylor College of Medicine.
Use of this software is governed by a license.
See the included file license.txt for details.
=end

# Plugin's routes
# See: http://guides.rubyonrails.org/routing.html
post '/projects/:id/genboree_registry_settings/update', :to => "genboree_registry_settings#update", :as => :genboree_registry_settings_update
#post '/projects/:id/genboree_registry/create', :to => "genboree_registry_entry#create", :as => :genboree_registry_entry_path
get  '/projects/:id/genboree_registry/create', :to => "genboree_registry_entry#create", :as => :genboree_registry_entry_path


#initial version of register file - obsolete now
#get '/projects/:id/genboree_registry/file_register', :to => "genboree_registry_entry#file_register", :as => :genboree_registry_file_register_path
#post '/projects/:id/genboree_registry/post_file', :to => "genboree_registry_entry#post_file", :as => :genboree_registry_post_file_path
#get '/projects/:id/genboree_registry/show', :to => "genboree_registry_entry#show", :as => :genboree_registry_show_path

# routes for just querying and showing allele information
get '/projects/:id/genboree_registry/allele',       :to => "genboree_registry_query#allele",       :as => :genboree_registry_allele_path
get '/projects/:id/genboree_registry/alleles',       :to => "genboree_registry_query#alleles",       :as => :genboree_registry_alleles_path
get '/projects/:id/genboree_registry/by_caid',       :to => "genboree_registry_query#by_caid",       :as => :genboree_registry_by_caid_path
get '/projects/:id/genboree_registry/by_cnvid',       :to => "genboree_registry_query#by_cnvid",       :as => :genboree_registry_by_cnvid_path
get '/projects/:id/genboree_registry/by_canonicalid',       :to => "genboree_registry_query#by_canonicalid",       :as => :genboree_registry_by_canonicalid_path
#get '/projects/:id/genboree_registry/single_allele', :to => "genboree_registry_query#single_allele", :as => :genboree_registry_single_allele_path
#get '/projects/:id/genboree_registry/vector_allele', :to => "genboree_registry_query#vector_allele", :as => :genboree_registry_vector_allele_path

# The landing page for public
get '/projects/:id/genboree_registry/index', :to => "genboree_registry_entry#index", :as => :genboree_registry_index

# generated assited HGVS
get '/projects/:id/assisted_hgvs/from_gene',         :to => "assisted_hgvs#from_gene"        , :as => :assisted_hgvs_from_gene
get '/projects/:id/assisted_hgvs/generate_hgvs',     :to => "assisted_hgvs#generate_hgvs" , :as => :assisted_hgvs_generate_hgvs
get '/projects/:id/assisted_hgvs/render_transcripts',     :to => "assisted_hgvs#render_transcripts" , :as => :assisted_hgvs_render_transcripts
post '/projects/:id/assisted_hgvs/show_transcripts',  :to => "assisted_hgvs#show_transcripts"             , :as => :assisted_hgvs_show_transcripts
get '/projects/:id/assisted_hgvs/render_sequence',   :to => "assisted_hgvs#render_sequence" , :as => :assisted_hgvs_render_sequence


# Landing page
get '/projects/:id/genboree_registry/landing',         :to => "genboree_registry_query#landing"        , :as => :genboree_registry_landing

# bulk query using form
get  '/projects/:id/bulk_query/start', :to => "bulk_query#start"    , :as => :bulk_query
post '/projects/:id/bulk_query/start', :to => "bulk_query#initiate" 

# gene:c./p. to HGVS
get  '/projects/:id/gene_to_hgvs', :to => "gene_to_hgvs#new"    , :as => :gene_to_hgvs
post '/projects/:id/gene_to_hgvs', :to => "gene_to_hgvs#create" 
# Two more routes to support Gene based indel queries
get  '/projects/:id/gene_indel', :to => "gene_to_hgvs#indel_new"    , :as => :gene_and_indel
post '/projects/:id/gene_indel', :to => "gene_to_hgvs#indel_create" 
