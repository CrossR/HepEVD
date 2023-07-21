//
// Rendering State
//

import * as THREE from "three";
import { OrbitControls } from "three/addons/controls/OrbitControls.js";

import { fitSceneInCamera, setupControls } from "./camera_and_controls.js";
import { BUTTON_ID, HIT_CONFIG } from "./constants.js";
import { getHitClasses, getHitProperties, getMCColouring } from "./helpers.js";
import { drawHits } from "./hits.js";
import { drawBox } from "./rendering.js";
import {
  enableMCToggle,
  isButtonActive,
  populateClassToggle,
  populateDropdown,
  toggleButton,
  updateUI,
} from "./ui.js";

/**
 * Represents the state of the rendering process, including the scene, camera,
 * detector geometry, and hit groups.
 */
export class RenderState {
  // Setup some basics, the scenes, camera, detector and hit groups.
  constructor(name, camera, renderer, hits, mcHits, geometry) {
    this.name = name;
    this.hitType = name;

    // THREE.js Setup...
    this.scene = new THREE.Scene();
    this.camera = camera;
    this.controls = new OrbitControls(this.camera, renderer.domElement);

    // Setup various groups for rendering into...
    this.detGeoGroup = new THREE.Group();
    this.hitGroup = new THREE.Group();
    this.mcHitGroup = new THREE.Group();

    this.scene.add(this.detGeoGroup);
    this.scene.add(this.hitGroup);
    this.scene.add(this.mcHitGroup);

    // Data Setup, first the top level static arrays...
    this.detectorGeometry = geometry;
    this.hits = hits;
    this.mcHits = mcHits;

    // The generated property lists...
    this.hitProperties = getHitProperties(this.hits);
    this.hitClasses = getHitClasses(this.hits);

    // Finally, setup the dynamic bits, the state that will change.
    this.uiSetup = false;

    this.activeHits = this.hits;
    this.activeMC = this.mcHits;
    this.activeHitColours = [];

    this.activeHitProps = new Set([BUTTON_ID.All]);
    this.activeHitClasses = new Set();

    this.otherRenderer = undefined;
  }

  /**
   * Returns the number of hits in the current state.
   * @returns {number} The number of hits.
   */
  get hitSize() {
    return this.hits.length;
  }

  /**
   * Returns a boolean indicating whether the scene is currently visible.
   * @returns {boolean} Whether the scene is visible.
   */
  get visible() {
    return this.scene.visible;
  }

  /**
   * Renders the detector geometry for the current state. Currently only renders
   * box geometry.
   */
  renderGeometry() {
    this.detGeoGroup.clear();

    // For now, just render the box geometry and nothing else.
    const boxVolumes = this.detectorGeometry.filter(
      (volume) => volume.type === "box",
    );
    boxVolumes.forEach((box) =>
      drawBox(this.hitType, this.detGeoGroup, this.hits, box),
    );
  }

  /**
   * Renders the hits for the current state, based on the active hit classes and properties.
   * Clears the hit group and then draws the hits with the active hit colours.
   */
  renderHits() {
    this.hitGroup.clear();

    drawHits(
      this.hitGroup,
      this.activeHits,
      this.activeHitColours,
      HIT_CONFIG[this.hitType],
    );
  }

  /**
   * Renders the hits for the current state, based on the active hit classes and properties.
   * Clears the hit group and then draws the hits with the active hit colours.
   */
  renderMCHits() {
    this.mcHitGroup.clear();

    const mcColours = getMCColouring(this.activeMC);

    drawHits(
      this.mcHitGroup,
      this.activeMC,
      mcColours,
      HIT_CONFIG[this.hitType],
    );
  }

  /**
   * Updates the active hits and hit colours based on the current active hit
   * classes and properties.
   */
  #updateHitArrays() {

    let newHits = [];
    let newMCHits = [];
    let newHitColours = [];

    // First, do the actual hits...
    this.hits.forEach((hit) => {
      if (
        this.activeHitClasses.size > 0 &&
        !this.activeHitClasses.has(hit.class)
      )
        return;
      this.activeHitProps.forEach((property) => {
        if (!this.hitProperties.get(hit).has(property)) return;
        newHits.push(hit);
        newHitColours.push(this.hitProperties.get(hit).get(property));
      });
    });

    // Then repeat for the MC hits, but skip the hit properties bit.
    this.mcHits.forEach((hit) => {
      if (
        this.activeHitClasses.size > 0 &&
        !this.activeHitClasses.has(hit.class)
      )
        return;
      newMCHits.push(hit);
    });

    this.activeHits = newHits;
    this.activeHitColours = newHitColours;
    this.activeMC = newMCHits;
  }

  // What to do if the hit property option changes:
  //  - Update the active hit properties list.
  //  - We need to update any UI around them.
  //  - Can then re-render the hits out.
  onHitPropertyChange(hitProperty) {

    // If the "None" button is clicked, short-circuit.
    // Clear everything out, re-render, stop.
    if (hitProperty === BUTTON_ID.None) {
        this.activeHitProps.clear();
        this.#updateHitArrays();
        toggleButton(this.hitType, hitProperty);
        this.toggleScene(this.hitType);
        this.renderHits();

        return;
    }

    // Similarly, if there was no actual hitProperty, i.e. this
    // is just a scene toggle, just swap the scene and finish.
    if (hitProperty === "") {
        this.toggleScene(this.hitType);
        return;
    }

    const buttonActive = isButtonActive(this.hitType, hitProperty);
    const sceneActive = this.scene.visible;

    // If the button is active, but the scene is not, just enable the scene.
    if (buttonActive && !sceneActive) {
      this.toggleScene(this.hitType);
      return;
    }

    // Add or remove the toggled property as needed...
    if (this.activeHitProps.has(hitProperty)) {
      this.activeHitProps.delete(hitProperty);
    } else {
      this.activeHitProps.add(hitProperty);
    }

    // Fix the active hits for this change...
    this.#updateHitArrays();

    // Now that the internal state is correct, correct the UI.
    toggleButton(this.hitType, hitProperty);
    this.toggleScene(this.hitType);

    // Finally, render the new hits!
    this.renderHits();
  }

  // Similar to the property change, update the hit class list.
  onHitClassChange(hitClass) {
    // Add or remove the toggled class as needed...
    if (this.activeHitClasses.has(hitClass)) {
      this.activeHitClasses.delete(hitClass);
    } else {
      this.activeHitClasses.add(hitClass);
    }

    // Fix the active hits for this change...
    this.#updateHitArrays();

    // Now that the internal state is correct, correct the UI.
    toggleButton("classes", hitClass, false);
    this.toggleScene(this.hitType);

    // Finally, render the new hits!
    this.renderHits();
  }

  // Finally, a MC-hit based toggle, enabling or disabling as needed.
  onMCToggle() {

    // Toggle the visibility state.
    this.mcHitGroup.visible = ! this.mcHitGroup.visible;

    // Now that the internal state is correct, correct the UI.
    toggleButton("classes_MC_toggle", this.hitType, false);

    // Render out the new hits.
    this.renderMCHits();
  }

  // Setup the UI, should only be called once.
  // Populate various drop downs and buttons based on the state,
  // and then reset the camera.
  setupUI(renderTarget) {
    if (this.uiSetup) return;

    this.renderGeometry();
    this.renderHits();

    // Also render out the MC, but its not visible at first.
    this.renderMCHits();
    this.mcHitGroup.visible = false;

    // Fill in any dropdown entries, or hit class toggles.
    populateDropdown(this.hitType, this.hitProperties, (prop) =>
      this.onHitPropertyChange(prop),
    );
    populateClassToggle(this.hitType, this.hits, (hitClass) =>
      this.onHitClassChange(hitClass),
    );
    enableMCToggle(this.hitType, this.mcHits, () => this.onMCToggle());

    // Move the scene/camera around to best fit it in.
    fitSceneInCamera(
      this.camera,
      this.controls,
      this.detGeoGroup,
      this.hitType,
    );
    setupControls(this.hitType, this.controls);
    this.scene.add(this.camera);

    // Setup the default button.
    toggleButton(this.hitType, BUTTON_ID.All);

    this.toggleScene(renderTarget);
    this.uiSetup = true;
  }

  // Attempt to activate or deactivate the scene, if needed.
  toggleScene(renderTarget) {
    this.scene.visible = this.hitType === renderTarget;
    this.controls.enabled = this.hitType === renderTarget;

    this.otherRenderer.scene.visible =
      this.otherRenderer.hitType === renderTarget;
    this.otherRenderer.controls.enabled =
      this.otherRenderer.hitType === renderTarget;

    updateUI(renderTarget);
  }
}
