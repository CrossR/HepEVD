//
// Current Hit Type State
//
// A top level state object that keeps track of the current hit types.
// This is used by pretty much every other state object to determine what to show.

import { getHitTypes } from "./helpers.js";

export class HitTypeState {
  constructor(particles, hits) {
    this.types = getHitTypes(particles, hits);

    this.activeTypesUI = new Set();
    this.activeTypesFilter = new Set();
  }

  get activeTypes() {
    return new Set([...this.activeTypesUI, ...this.activeTypesFilter]);
  }

  /**
   * Toggles the active state of a hit type.
   *
   * @param {string} type - The hit type to toggle.
   */
  toggleHitType(type) {
    if (this.activeTypesUI.has(type)) {
      this.activeTypesUI.delete(type);
    } else {
      this.activeTypesUI.add(type);
    }
  }

  /**
   * Set the active state of a hit type.
   *
   * @param {string} type - The hit type to toggle.
   * @param {boolean} active - The active state.
   */
  addHitType(type, active = true) {
    if (! this.activeTypesFilter.has(type) && active)
      this.activeTypesFilter.add(type);

    if (this.activeTypesFilter.has(type) && ! active)
        this.activeTypesFilter.delete(type);

    console.log("Active types: ", this.activeTypes);
  }

  /**
   * Checks if the hit type is active.
   *
   * @param {Object} data - The hit data.
   * @returns {boolean} - True if the hit type is active, false otherwise.
   */
  checkHitType(data) {
    return (
      this.activeTypes.size === 0 || this.activeTypes.has(data.position.hitType)
    );
  }
}
