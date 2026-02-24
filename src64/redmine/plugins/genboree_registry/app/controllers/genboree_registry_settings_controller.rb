class GenboreeRegistrySettingsController < ApplicationController

  unloadable

  before_filter  :find_project
  before_filter  :set_project_settings

  skip_before_filter :check_if_login_required
  skip_before_filter :verify_authenticity_token

  accept_api_auth :index, :create, :delete

  def update
    #$stderr.puts "params: #{params.inspect}"
    userRedmineLayout = ( params['userRedmineLayout']['aa'] == '0' ? false : true )
    gbHost = params['gbHost']
    #projectId = params['project_id']
    calculatorPath = params['calculatorPath']
    cnvHost = params['cnvHost'] || ""
    isProduction   = params['isProduction']['answer'] 

    logger.info "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
    logger.info params.inspect
    logger.info isProduction
    logger.info "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"

    genboreeRegistryRec = GenboreeRegistry.find_by_project_id(@project.id)

    if(genboreeRegistryRec.nil?) # Put in a new entry
      GenboreeRegistry.create( 
			       :gbHost => gbHost , 
                               :project_id => @project.id, 
                               :userRedmineLayout => userRedmineLayout,
                               :calculatorPath => calculatorPath,
			       :isProduction => isProduction,
             :cnvHost => cnvHost
			     )
      flash[:notice] = "Settings created. The host is #{gbHost}"
    else # update existing record
      genboreeRegistryRec.gbHost = gbHost
      genboreeRegistryRec.project_id = @project.id
      genboreeRegistryRec.userRedmineLayout = userRedmineLayout
      genboreeRegistryRec.calculatorPath = calculatorPath
      genboreeRegistryRec.isProduction   = isProduction
      genboreeRegistryRec.cnvHost   = cnvHost
      genboreeRegistryRec.save
      flash[:notice] = "Settings updated."
    end


    kbMount = RedmineApp::Application.routes.default_scope[:path]

    redirect_to "#{kbMount}/projects/#{@project}/settings/genboree_registry_settings"

  end

  # ------------------------------------------------------------------
  # PRIVATE HELPERS
  # ------------------------------------------------------------------

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
