class AddCnvHostToGenboreeRegistries < ActiveRecord::Migration
  def change
    add_column :genboree_registries, :cnvHost, :string
  end
end
