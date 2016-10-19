#!/usr/bin/env ruby
# coding: utf-8

TMP_DIR = '/tmp'
EXE_BIN = '/usr/local/bin/svg2emb'
DEFAULT_FORMAT = '.pes'
ACCEPT_FORMATS = [ '.dst', '.hus', '.jef', '.pes' ]

require 'cgi'


cgi = CGI.new

# check output format
outfmt = cgi.params['out_format']
if outfmt.is_a?(String) then
  # check correct format
  outfmt.downcase!
  if ! ACCEPT_FORMATS.include?(outfmt) then
    # invalid format
    outfmt = DEFAULT_FORMAT
  end
else
  # format not set in HTML form
  outfmt = DEFAULT_FORMAT 
end

uploadsvg = cgi.params['svgfile'][0]

svgfile = File.join(TMP_DIR, "#{$$}.svg")
embfile = File.join(TMP_DIR, "#{$$}#{outfmt}")

begin
  open(svgfile, 'wb') do |f|
    f.write(uploadsvg.read)
  end

  pipeout = IO.popen([EXE_BIN, svgfile, embfile], 'r') { |io|
    io.gets
  }
  if $?.success? then
    # convert succeeded
    open(embfile, 'rb') do |f|
      print "Content-Type: application/octet-stream\r\n"
      print "Content-Length: #{f.size}\r\n"
      print "Content-Disposition: attachment; filename=\"convert#{outfmt}\"\r\n"
      print "\r\n"
      print f.read
    end
  else
    # failed to convert
    print "Status: 500\r\n"
    print "Content-Type: text/plain\r\n"
    print "\r\n"
    print "Convert failed.\r\n"
    print "--------\r\n"
    print pipeout

  end

ensure
  # remove files
  [svgfile, embfile].each do |rmfile|
    begin
      File.unlink(rmfile)
    rescue
      # ignore
    end
  end
end