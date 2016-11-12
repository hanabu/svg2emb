#!/usr/bin/env ruby
# coding: utf-8

require 'cgi'


TMP_DIR = '/tmp'
EXE_SVG2EMB = '/usr/local/bin/svg2emb'
EXE_FZ2EMB  = '/usr/local/bin/fz2emb'

DEFAULT_FORMAT = '.pes'
ACCEPT_FORMATS = [ '.dst', '.hus', '.jef', '.pes', '.svg' ]

DEFAULT_MIME = 'application/octet-stream'
FORMAT2MIMES = { '.svg' => 'image/svg+xml' }

DEFAULT_MODE = 'normal'
ACCEPT_MODES = [ 'normal', 'fritzing09' ]

# timestamp (ISO8601 basic format)
timestamp = Time.now.localtime.strftime('%Y%m%dT%H%M%S')

cgi = CGI.new


# check output format
outfmt = cgi.params['out_format']
if outfmt.is_a?(Array) && outfmt[0].is_a?(String) then
  # check correct format
  outfmt = outfmt[0].downcase
  if ! ACCEPT_FORMATS.include?(outfmt) then
    # invalid format
    outfmt = DEFAULT_FORMAT
  end
else
  # format not set in HTML form
  outfmt = DEFAULT_FORMAT 
end

# output mime format
outmime = FORMAT2MIMES[outfmt]
outmime = DEFAULT_MIME unless outmime
if outmime.start_with?('application') then
  cdisp = 'attachment'
else
  cdisp = 'inline'
end

# check mode
mode = cgi.params['mode']
if mode.is_a?(Array) && mode[0].is_a?(String) then
  mode = mode[0].downcase
  if ! ACCEPT_MODES.include?(mode) then
    # invalid mode
    mode = DEFAULT_MODE
  end
else
  # mode not set in HTML form
  mode = DEFAULT_MODE
end



uploadsvg = cgi.params['file'][0]

tmpfile = File.join(TMP_DIR, "#{timestamp}_#{$$}.in")
embfile = File.join(TMP_DIR, "#{timestamp}_#{$$}#{outfmt}")

begin
  inputtype = :unknown

  open(tmpfile, 'wb') do |f|
    loop do
      buf = uploadsvg.read(1024)
      if buf then
        f.write(buf)

        if :unknown == inputtype then
          # check file type
          if buf.start_with?('PK') then
            # ZIP compressed, may be Fritzing *.fzz
            inputtype = :fzz
          else
            # not Fritzing, assume SVG
            inputtype = :svg
          end
        end
      else
        # eof
        break
      end
    end
  end

  if :fzz == inputtype then
    # Fritzing to embroidery direct convert
    pipeout = \
    IO.popen("funzip #{tmpfile} | #{EXE_FZ2EMB} #{embfile}", 'r') { |io|
      io.gets      
    }
  else
    pipeout = \
    IO.popen([EXE_SVG2EMB, '-m', mode, tmpfile, embfile], 'r') { |io|
      io.gets
    }
  end

  if $?.success? then
    # convert succeeded
    open(embfile, 'rb') do |f|
      print "Content-Type: #{outmime}\r\n"
      print "Content-Length: #{f.size}\r\n"
      print "Content-Disposition: #{cdisp}; filename=\"convert#{outfmt}\"\r\n"
      print "\r\n"
      print f.read
    end
  else
    # failed to convert
    print "Status: 400\r\n"
    print "Content-Type: text/plain\r\n"
    print "\r\n"
    print "Convert failed.\r\n"
    print "--------\r\n"
    print pipeout

  end

ensure
  # remove files
  [tmpfile, embfile].each do |rmfile|
    begin
      File.unlink(rmfile)
    rescue
      # ignore
    end
  end
end
