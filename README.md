# mruby extension module for MIRACLE ZBX

mrbuy embedded loadable module for MIRACLE ZBX.
It adds "mruby.eval[]" and "mruby.module[]" key.

"mruby.eval[]" evaluate argument string as mruby code and return the result.
"mruby.module[]" executes functions in mruby file and return the result.

## Compile

    $ C_INCLUDE_PATH="/path/to/miracle-zbx/include" make

## Sample

'mruby.eval[3+5]' returns '8.000000'
'mruby.eval[p "hello world"]' returns 'hello world'

'mruby.module[sample.rb]' executes zbx_module_run() in MonitoringModule class declared in sample.rb.
zbx_module_init() and zbx_module_uninit() are also needed in MonitoringModule class.
They are executed in agent starting and stopping.
zbx_module_run() can have arguments by adding arguments to monitoring key like 'mruby.module[sample.rb, Hi]'.

See 'mruby_module/sample.rb' for more detail.
