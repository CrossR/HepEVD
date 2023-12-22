//
// MC Data State
//

export class MCDataState {
  constructor(mcHits) {
    this.mcHits = mcHits;
    this.activeMC = [];
  }

  // Property accessors
  get mc() {
    return this.activeMC;
  }

  updateActive(hitTypeState) {
    this.activeMC = this.mcHits.filter((mcHit) => {
      // Skip if hit type is not active
      if (!hitTypeState.checkHitType(mcHit)) return false;

      return true;
    });
  }
}
