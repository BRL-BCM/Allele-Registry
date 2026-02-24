require 'uri'
require 'yaml'
require 'json'
require 'mysql2'
require 'em-http-request'
require 'brl/util/util'
require 'brl/db/dbrc'
require 'brl/rest/apiCaller'
require 'brl/cache/helpers/dnsCacheHelper'
require 'brl/cache/helpers/domainAliasCacheHelper'
require 'brl/genboree/kb/kbDoc'
require 'brl/genboree/kb/propSelector'
require 'brl/genboree/kb/producers/abstractTemplateProducer'

require_dependency 'genboree_registry/hooks'

Redmine::Plugin.register :genboree_registry do
  name 'Genboree Registry plugin'
  author 'Ronak Patel'
  description 'This is a plugin for Redmine'
  version '__ALLELE_REGISTRY_VERSION__'
  url 'http://example.com/path/to/plugin'
  author_url 'http://example.com/about'

  # These are the permission
  # Currently only registered users can see
  project_module(:genboree_registry) {
      permission :register_allele, :genboree_registry_entry => [:create, :file_register, :post_file, :allele, :alleles , :index], :genboree_registry_query => [ :allele, :alleles, :index, :landing ]  , :assisted_hgvs => [:from_gene, :generate_hgvs, :show_transcripts], :bulk_query => [:start, :initiate]
      permission :query_allele , :genboree_registry_query => [ :allele, :alleles, :index, :landing ]  , :assisted_hgvs => [:from_gene, :generate_hgvs, :show_transcripts], :bulk_query => [:start, :initiate]
  }

  settings  :partial => 'settings/genboree_registry',
            :default =>
            {
              'path'                  => '/var/genboree_registry/{PROJECT}', 
              'index'                 => 'index.html',
              'menu'                  => 'Registry'
            }

  menu :project_menu, :genboree_registry,
    {
      :controller => "genboree_registry_entry",
      :action => "show"
    },
    :caption => Proc.new {  Setting.plugin_genboree_registry['menu'] },
    :if      => Proc.new { !Setting.plugin_genboree_registry['menu'].blank? }

end
