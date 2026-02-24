class AddColumnsToGenboreeRegistries < ActiveRecord::Migration
  def change
    add_column :genboree_registries, :calculatorPath, :string
    add_column :genboree_registries, :isProduction  , :boolean
  end
end
