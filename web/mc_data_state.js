//
// MC Data State
//

import { HitTypeState } from "./hit_type_state.js";

export class MCDataState {
  constructor(mcHits) {
    this.mcHits = mcHits;
    this.activeMC = [];
  }

  /**
   * Get the active MC, if any.
   *
   * @returns {any} The active MC.
   */
  get mc() {
    return this.activeMC;
  }

  /**
   * Top level update function, to update what the active MC hits are.
   *
   * @param {HitTypeState} hitTypeState - The hit type state object.
   */
  updateActive(hitTypeState) {
    this.activeMC = this.mcHits.filter((mcHit) => {
      // Skip if hit type is not active
      if (!hitTypeState.checkHitType(mcHit)) return false;

      return true;
    });
  }
}
