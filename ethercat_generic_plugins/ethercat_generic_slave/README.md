# EL6224EcSlave Plugin

`EL6224EcSlave` is a small plugin that extends `GenericEcSlave` with the Beckhoff EL6224-specific complete-access SDO fallback.

## Why this plugin exists

`GenericEcSlave` is intentionally generic: it can load arbitrary EtherCAT slave descriptions from YAML and map PDOs to ROS 2 control interfaces without knowing anything about a specific device.

The Beckhoff EL6224 IO-Link master is an exception because some of its channel configuration objects reject complete-access SDO downloads on Linux. When the driver writes those objects during startup, the normal CA SDO path can fail even though the same configuration works when replayed as individual per-subindex SDO writes.

If that workaround lived inside `GenericEcSlave`, the "generic" plugin would no longer be generic. Keeping the EL6224 logic in a dedicated plugin preserves the separation between:

- generic EtherCAT slave loading and PDO mapping
- device-specific protocol quirks and workarounds

## What it does

`EL6224EcSlave` overrides `getCaSdoFallback()` and returns a fallback handler that:

- only applies to EL6224 channel configuration objects in the `0x8000..0x8030` range
- only applies to 20-byte complete-access payloads
- replays the write as individual SDO downloads for the affected subindices

For any other index or payload size, the fallback returns `-1` and leaves the normal driver path unchanged.

## When to use it

Use this plugin only for EL6224 modules. In your `ec_module` entry, select:

```xml
<plugin>ethercat_generic_plugins/EL6224EcSlave</plugin>
```

For all other devices, keep using:

```xml
<plugin>ethercat_generic_plugins/GenericEcSlave</plugin>
```

## Example

In the EL6224 test-drive example, the module is configured like this:

```xml
<ec_module name="EL6224">
  <plugin>ethercat_generic_plugins/EL6224EcSlave</plugin>
  <param name="alias">0</param>
  <param name="position">2</param>
  <param name="slave_config">$(find ethercat_slave_description)/config/beckhoff/beckhoff_el6224.yaml</param>
</ec_module>
```

## Validation

This plugin is covered by unit tests that verify:

- the plugin can be loaded through `pluginlib`
- `GenericEcSlave` does not expose a CA fallback
- `EL6224EcSlave` does expose a CA fallback
- the fallback rejects unsupported payload sizes and indexes without needing hardware
