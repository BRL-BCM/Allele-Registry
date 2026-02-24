class GenboreeRegistryEntryController < ApplicationController

  unloadable

  layout 'registry_bootstrap_extensive_hdr_ftr'

  before_filter :find_project , :project_specific_authorize , :set_project_settings

  puts "Trying to print something before filter"

  skip_before_filter :check_if_login_required
  skip_before_filter :verify_authenticity_token

  accept_api_auth :create

  def single_allele
  end
  # This method registers allele using redmine session
  def create
    rackEnv = env
    #targetHost = GenboreeRegistry.last.gbHost
    targetHost = GenboreeRegistry.find_by_project_id(@project.id).gbHost

    @hgvs = params["hgvs"]
    escaped = CGI.escape(@hgvs)
    #escaped = @hgvs
    targetPath = "/allele?hgvs=#{escaped}"

    @response_from_registry = String.new()

 #   @apiReq = GbApi::SimpleAsyncApiRequester.new(rackEnv, targetHost, @project)
    @apiReq = GbApi::SimpleAsyncApiRequester.new(rackEnv, targetHost, nil)

    @apiReq.respCallback {|in_array|
      logger.error "REGISTRY - create respCallBack"
      in_array[2].each { |chunk|
           @response_from_registry << chunk
      }
    }

    @apiReq.bodyFinish {
      logger.error @response_from_registry
      logger.error "REGISTRY - create body finish "
      status = 200
      headers = {}
      @apiReq.renderToClient( self, :single_allele )
    }

    abc = @apiReq.put( targetPath, { } )

  end

  # This method shows the form for bulk HGVS registration
  def file_register
  end
 
  # This method shows the output of bulk registration
  def show
  end

  # This method takes file and sends put request
  def post_file
    file_in =  params['allele_file']
    @contents = file_in.tempfile.read
    rackEnv = env
    #targetHost = GenboreeRegistry.last.gbHost 
    targetHost = GenboreeRegistry.find_by_project_id(@project.id).gbHost
    targetPath = "/alleles?file=hgvs"
    @apiReq = GbApi::SimpleAsyncApiRequester.new(rackEnv, targetHost, @project)
    logger.error "#{@apiReq.inspect}"
    @response_from_registry = String.new()

    @apiReq.respCallback {|in_array|
      logger.error "inside respCallBack and this is the in array #{in_array}"
      in_array[2].each { |chunk| 
           @response_from_registry << chunk
      }
    }

    @apiReq.bodyFinish {
       status = 200
       headers = {}
       logger.error "#{@response_from_registry}"
       @apiReq.renderToClient(self, :show)
    }

    @apiReq.put( targetPath, {}, payload = @contents )
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
 
  def project_specific_authorize
    #$stderr.debugPuts(__FILE__, __method__, 'DEBUG', "++++++ CREATE CLINGEN USER: controler-action => #{@curr_action}")
    #$stderr.debugPuts(__FILE__, __method__, 'DEBUG', "++++++ CREATE CLINGEN USER: current user => #{@currRmUser.inspect}")
    allowed = Redmine::AccessControl.permissions.select{ |xx|
      xx.actions.include? (@curr_action)
    }.any?{|perm|
      #User.current.roles_for_project(@project).any?{|role|
      @currRmUser.roles_for_project(@project).any?{|role|
	role.permissions.include?(perm.name)
      }
    } rescue false
    #$stderr.debugPuts(__FILE__, __method__, 'DEBUG', "++++++ CREATE CLINGEN USER: User #{@currRmUser.inspect} allowed for #{@curr_action} => #{allowed}")

    if ! allowed
      #$stderr.debugPuts(__FILE__, __method__, 'DEBUG', "++++++ CREATE CLINGEN USER: Before rendering 403")
      authorize
      #render_403 :message => :permission_denied
    end
      #$stderr.debugPuts(__FILE__, __method__, 'DEBUG', "++++++ CREATE CLINGEN USER: At the end of method.")
  end

end
