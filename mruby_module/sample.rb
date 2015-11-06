class MonitoringModule
  def zbx_module_init()
    @env = {}
    @env['lmc'] = 'mlzbx_foo'
    @env['semlock_path'] = '/usr/lib64/zabbix/modules/mruby_module/sample.rb'
    @semlock = Semlock.new(@env['semlock_path'], 0, 1, 0600)
    @info = Cache.new 'namespace'=>@env['lmc'], 'min_alloc_size' => 512
    @semlock.lock(0)
    if @info['cup'] == nil then
      @info['cup'] = Marshal.dump(0)
    end
    @semlock.unlock(0)
  end
  def zbx_module_uninit()
    Cache.drop 'namespace'=>@env['lmc']
    @semlock.remove
  end
  def zbx_module_run(greeting='Hello world.')
    @semlock.lock(0)
    cups_num = Marshal.load(@info['cup']) + 1
    @info['cup'] = Marshal.dump(cups_num)
    @semlock.unlock(0)
    return greeting + " I drunk " + cups_num.to_s + " cups of water"
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
