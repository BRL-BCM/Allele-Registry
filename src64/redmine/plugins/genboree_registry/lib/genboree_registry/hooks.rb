#!/usr/bin/env ruby


module GenboreeRegistrySettingsHook
  class Hooks < Redmine::Hook::ViewListener
    def helper_projects_settings_tabs(context = {})
      context[:tabs].push({ 
	                  :name    => 'genboree_registry_settings',
                          :action  => :genboree_registry_settings,
                          :partial => 'projects/settings/genboree_registry_settings',
                          :label   => :gbregistry_label_project_settings_tab })
    end
  end
end
