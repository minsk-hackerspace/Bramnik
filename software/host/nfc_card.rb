require 'active_record'

class NFCCard < ActiveRecord::Base
  belongs_to :user
end