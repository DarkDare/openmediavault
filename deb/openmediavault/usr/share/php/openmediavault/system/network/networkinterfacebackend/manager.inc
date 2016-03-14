<?php
/**
 * This file is part of OpenMediaVault.
 *
 * @license   http://www.gnu.org/licenses/gpl.html GPL Version 3
 * @author    Volker Theile <volker.theile@openmediavault.org>
 * @copyright Copyright (c) 2009-2016 Volker Theile
 *
 * OpenMediaVault is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * OpenMediaVault is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenMediaVault. If not, see <http://www.gnu.org/licenses/>.
 */
namespace OMV\System\Network\NetworkInterfaceBackend;

require_once("openmediavault/globals.inc");

/**
 * Helper class to get all existing Linux network interfaces.
 * @ingroup api
 */
class Manager {
	private $backends = array();

	/**
	 * Returns a manager singleton.
	 * @return The manager object.
	 */
	public static function &getInstance() {
		static $instance = NULL;
		if (!isset($instance))
			$instance = new Manager();
		return $instance;
	}

	/**
	 * Register a storage device backend.
	 * @param backend The storage device backend object to register.
	 * @return TRUE if successful, otherwise FALSE.
	 */
	final public function registerBackend(BackendAbstract $backend) {
		if (!isset($backend))
			return FALSE;
		$this->backends[] = $backend;
		return TRUE;
	}

	/**
	 * Get the backend of the given network interface.
	 * @param deviceName Specifies the device name, e.g.
	 *   <ul>
	 *   \li eth0
	 *   \li wlan2
	 *   \li eth2.1
	 *   \li bond0
	 *   </ul>
	 * @return The backend that implements the given device, otherwise NULL.
	 */
	final public function getBackend($deviceName) {
		$result = NULL;
		foreach ($this->backends as $backendk => $backendv) {
			if (FALSE === $backendv->isTypeOf($deviceName))
				continue;
			$result = $backendv;
			break;
		}
		return $result;
	}

	/**
	 * Enumerate all network interfaces.
	 * @param type Defines the network interface type, e.g. ethernet, vlan or
	 *   bond. Defaults to OMV_NETWORK_INTERFACE_TYPE_ALL.
	 * @return A list of network interfaces, e.g. eth0 or lo, otherwise FALSE.
	 */
	final public function enumerate($type = OMV_NETWORK_INTERFACE_TYPE_ALL) {
		$result = array();
		foreach ($this->backends as $backendk => $backendv) {
			if (!($type & $backendv->getType()))
				continue;
			if (FALSE === ($devs = $backendv->enumerate()))
				return FALSE;
			$result = array_unique(array_merge($result, $devs));
		}
		// Sort the devices using a "natural order" algorithm.
		if (!sort($result, SORT_NATURAL))
			return FALSE;
		return $result;
	}

	/**
	 * Enumerate all unused devices. This list contains all devices of the
	 * given type except the devices that are used by other network interfaces
	 * (e.g. bond interfaces).
	 * @param type Defines the network interface type. Defaults to
	 *   OMV_NETWORK_INTERFACE_TYPE_ALL.
	 * @return A list of network interfaces, otherwise FALSE.
	 */
	final public function enumerateUnused($type = OMV_NETWORK_INTERFACE_TYPE_ALL) {
		// Get all existing network interfaces of the given type.
		if (FALSE === ($result = $this->enumerate($type)))
			return FALSE;
		// Remove used network interfaces.
		foreach ($this->backends as $backendk => $backendv) {
// Always collect and remove all slave devices.
//			if (!($type & $backendv->getType()))
//				continue;
			if (FALSE === ($slaves = $backendv->enumerateSlaves()))
				return FALSE;
			if (empty($slaves))
				continue;
			$result = array_diff($result, $slaves);
		}
		// Sort the devices using a "natural order" algorithm.
		if (!sort($result, SORT_NATURAL))
			return FALSE;
		return $result;
	}

	/**
	 * Enumerate all used devices. The list contains all those devices that
	 * are used by the given network interfaces, e.g. all members of a bond
	 * interface.
	 * @param type Defines the network interface type. Defaults to
	 *   OMV_NETWORK_INTERFACE_TYPE_ALL.
	 * @return A list of network interfaces, otherwise FALSE.
	 */
	final public function enumerateUsed($type = OMV_NETWORK_INTERFACE_TYPE_ALL) {
		$result = array();
		foreach ($this->backends as $backendk => $backendv) {
			if (!($type & $backendv->getType()))
				continue;
			if (FALSE === ($slaves = $backendv->enumerateSlaves()))
				return FALSE;
			if (empty($slaves))
				continue;
			$result = array_unique(array_merge($result, $slaves));
		}
		// Sort the devices using a "natural order" algorithm.
		if (!sort($result, SORT_NATURAL))
			return FALSE;
		return $result;
	}

	/**
	 * Check if the given device is used by another network interface.
	 * @param deviceName Specifies the device name, e.g.
	 *   <ul>
	 *   \li eth0
	 *   \li wlan2
	 *   \li eth2.1
	 *   \li bond0
	 *   </ul>
	 * @param type Defines the network interface type. Defaults to
	 *   OMV_NETWORK_INTERFACE_TYPE_ALL.
	 * @return TRUE if the given device is used by another network
	 *   interface, otherwise FALSE.
	 */
	final public function isUsed($deviceName, $type = OMV_NETWORK_INTERFACE_TYPE_ALL) {
		$result = FALSE;
		foreach ($this->backends as $backendk => $backendv) {
			if (!($type & $backendv->getType()))
				continue;
			if (FALSE === ($slaves = $backendv->enumerateSlaves()))
				return FALSE;
			if (in_array($deviceName, $slaves)) {
				$result = TRUE;
				break;
			}
		}
		return $result;
	}

	/**
	 * Get the object of the class implementing the given filesystem.
	 * @param deviceName Specifies the device name, e.g.
	 *   <ul>
	 *   \li eth0
	 *   \li wlan2
	 *   \li eth2.1
	 *   \li bond0
	 *   </ul>
	 * @return The object of the class implementing the given network
	 *   interface, otherwise NULL.
	 */
	final public function getImpl($deviceName) {
		if (NULL == ($backend = $this->getBackend($deviceName)))
			return NULL;
		return $backend->getImpl($deviceName);
	}
}