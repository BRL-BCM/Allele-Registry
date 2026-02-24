#!/usr/bin/env ruby

require 'net/http'
require 'digest/sha1'


# return response
# throw exceptions in case of errors 
def putData(url, data, login, password)
    # calculate token & full URL
    identity = Digest::SHA1.hexdigest("#{login}#{password}")
    gbTime = Time.now.to_i.to_s
    token = Digest::SHA1.hexdigest("#{url}#{identity}#{gbTime}")
    request = "#{url}&gbLogin=#{login}&gbTime=#{gbTime}&gbToken=#{token}"
    # send request & parse response
    http = Net::HTTP.new(URI(url).host)
    http.read_timeout = 1200 # seconds
    req = Net::HTTP::Put.new("#{request}")
    req.body = data
    res = http.request(req)
    response = res.body
    # check status
    if not res.is_a? Net::HTTPSuccess
        raise "Error for PUT requests: #{response}"
    end
    return response
end


# return response
# throw exceptions in case of errors 
def postData(url, data)
    # send request & parse response
    http = Net::HTTP.new(URI(url).host)
    http.read_timeout = 1200 # seconds
    req = Net::HTTP::Post.new("#{url}")
    req.body = data
    res = http.request(req)
    response = res.body
    # check status
    if not res.is_a? Net::HTTPSuccess
        raise "Error for POST requests: #{response}"
    end
    return response
end


if ARGV.size != 2 and ARGV.size != 4
    STDERR.puts "Incorrect parameters!"
    puts "This program sends POST or PUT request with payload and print the response."
    puts "Parameters for POST request:  URL  file_with_payload"
    puts "Parameters for PUT  request:  URL  file_with_payload  login  password"
    exit 1
end

url=ARGV[0]
file=ARGV[1]
login=ARGV[2]
password=ARGV[3]

data = File.open(file).read
url = "http://" + url if (url !~ /^http:\/\// and url !~ /^https:\/\//)
response = nil

if login.nil? or password.nil?
  response = postData(url, data)
else
  response = putData(url, data, login, password)
end

puts "#{response}"
