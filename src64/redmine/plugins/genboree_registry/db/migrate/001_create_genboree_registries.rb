=begin
Copyright (c) 2001-2016 Baylor College of Medicine.
Use of this software is governed by a license.
See the included file license.txt for details.
=end

class CreateGenboreeRegistries < ActiveRecord::Migration
  def change
    create_table :genboree_registries do |t|
      t.string :gbHost
      t.integer :project_id
      t.boolean :userRedmineLayout,  :default => 1
    end
  end
end
