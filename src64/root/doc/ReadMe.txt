The content of this directory:

1) AlleleRegistry_X.XX.XX_api_vX.pdf - the API specification.

2) scripts - a folder with a ready-to-use example script to send 
   HTTP POST or HTTP PUT request with payload to the Allele Registry 
   server from Linux terminal. It can be used for bulk variant query or
   registration (see the API specification for more details). The script
   is available in three versions:
   -> request_with_payload.py - Python script, requires Python >= 2.7
                                and the library "request"
   -> request_with_payload.rb - Ruby script, requires Ruby >= 1.8.7
   -> request_with_payload.sh - Bash script, uses general console tools.

3) example_data - a folder with example input files for bulk query and 
   bulk registration of variants.
