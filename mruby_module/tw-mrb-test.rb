class MonitoringModule
  def zbx_module_init()
    CONSUMER_KEY        = ''
    CONSUMER_SECRET     = ''
    ACCESS_TOKEN        = ''
    ACCESS_TOKEN_SECRET = ''
    @env = {}
    @env['lmc'] = 'mlzbx_bar'
    @env['semlock_path'] = '/usr/lib64/zabbix/modules/mruby_module/tw-mrb-test.rb'
    @semlock = Semlock.new(@env['semlock_path'], 3, 2, 0600)
    @semlock.lock(0)
    @info = Cache.new('namespace'=>@env['lmc'], 'min_alloc_size' => 512)
    @semlock.unlock(0)
    @semlock.lock(0)
    if @info['twitter'] == nil then
      @info['twitter']=Marshal.dump(OAuth.new(CONSUMER_KEY, CONSUMER_SECRET, ACCESS_TOKEN, ACCESS_TOKEN_SECRET))
    end
    @semlock.unlock(0)
  end
  def zbx_module_uninit()
    Cache.drop 'namespace'=>@env['lmc']
    @semlock.remove
  end
  def zbx_module_run()
    GET_API_URL         = 'https://api.twitter.com/1.1/search/tweets.json?q=%s&count=%s'
    twitter = Marshal.load(@info['twitter'])
    tweet_num = 1
    response = twitter.get(GET_API_URL % ['ruby', tweet_num.to_s])
    if response.code.to_i == 200
      data = JSON::parse(response.body)
      data["statuses"].each do |d|
        return "#{d["created_at"]}: #{d["user"]["screen_name"]}'s tweet:  #{d["text"]}"
      end
    else
      puts response.body
      raise "Request failed: " + response.code.to_s
    end
  end
end

if $0 == __FILE__ then
  a = MonitoringModule.new
  a.zbx_module_init
  puts a.zbx_module_run
  puts a.zbx_module_run
  puts a.zbx_module_run
  a.zbx_module_uninit
end
