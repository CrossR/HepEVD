//
// Rendering State
//

import * as THREE from "three";
import { OrbitControls } from "three/addons/controls/OrbitControls.js";

import { drawBox, fitSceneInCamera } from "./rendering.js";
import { drawHits, setupControls } from "./hits.js";
import { getHitProperties, getHitClasses } from "./helpers.js";
import {
  populateClassToggle,
  populateDropdown,
  isButtonActive,
    toggleButton,
  updateUI,
} from "./ui.js";
import {
  BUTTON_ID,
  HIT_CONFIG,
} from "./constants.js";

// Class that stores the render state of the app.
export class RenderState {

    // Setup some basics, the scenes, camera, detector and hit groups.
    constructor(name, camera, renderer, hits, geometry) {

        this.name = name;
        this.hitType = name;

        // THREE.js Setup
        this.scene = new THREE.Scene();
        this.camera = camera;
        this.controls = new OrbitControls(this.camera, renderer.domElement);

        this.detGeoGroup = new THREE.Group();
        this.scene.add(this.detGeoGroup);
        this.hitGroup = new THREE.Group();
        this.scene.add(this.hitGroup);

        // Data Setup
        this.hits = hits;
        this.hitProperties = getHitProperties(this.hits);
        this.hitClasses = getHitClasses(this.hits);
        this.detectorGeometry = geometry;

        // State
        this.uiSetup = false;
        this.activeHits = this.hits;
        this.activeHitColours = [];
        this.activeHitProps = new Set([BUTTON_ID.All]);
        this.otherRenderer = undefined;
    }

    // How many hits are there total?
    get hitSize() {
        return this.hits.length;
    }

    // Get the scene visibility state.
    get visible() {
        return this.scene.visible;
    }

    // Clear, then render out the various geometries.
    renderGeometry() {
        this.detGeoGroup.clear();

        // For now, just render the box geometry and nothing else.
        const boxVolumes = this.detectorGeometry.filter((volume) => volume.type === "box");
        boxVolumes.forEach((box) =>
            drawBox(this.hitType, this.detGeoGroup, this.hits, box),
        );
    };

    // Clear, then render out the currently active hits.
    renderHits() {
        this.hitGroup.clear();

        drawHits(
            this.hitGroup,
            this.activeHits,
            this.activeHitColours,
            HIT_CONFIG[this.name]
        );
    }

    // What to do if the hit property option changes:
    //  - We want to rebuild the active hits and active hit colours arrays.
    //  - We need to update any UI around them.
    //  - Can then re-render the hits out.
    onHitPropertyChange(hitProperty) {

        const buttonActive = isButtonActive(this.hitType, hitProperty);
        const sceneActive = this.scene.visible;

        // If the button is active, but the scene is not, just enable the scene.
        if (buttonActive && !sceneActive) {
            this.toggleScene(this.hitType);
            updateUI();
            return;
        }

        // Otherwise, update all the hits to find the superset formed by all
        // the current active properties.
        let newHits = [];
        let newHitColours = [];

        if (this.activeHitProps.has(hitProperty)) {
            this.activeHitProps.delete(hitProperty);
        } else {
            this.activeHitProps.add(hitProperty);
        }

        this.hits.forEach((hit) => {
            this.activeHitProps.forEach((property) => {
                if (! this.hitProperties.get(hit).has(property)) return;
                newHits.push(hit);
                newHitColours.push(this.hitProperties.get(hit).get(property));
            });
        });

        this.activeHits = newHits;
        this.activeHitColours = newHitColours;

        // Now that the internal state is correct, correct the UI.
        toggleButton(this.hitType, hitProperty);
        this.toggleScene(this.hitType);

        this.renderHits();
    }

    // Setup the UI, should only be called once.
    // Populate various dropdowns and buttons based on the state,
    // and then reset the camera.
    setupUI(renderTarget) {

        if (this.uiSetup) return;

        // Fill in any dropdown entries, or hit class toggles.
        populateDropdown(this.name, this.hitProperties, (prop) => this.onHitPropertyChange(prop));
        populateClassToggle(this.name, this.hits, (a) => { console.log(a); });

        // Move the scene/camera around to best fit it in.
        fitSceneInCamera(
            this.camera,
            this.controls,
            this.detGeoGroup,
            this.hitType
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

        this.otherRenderer.scene.visible = this.otherRenderer.hitType === renderTarget;
        this.otherRenderer.controls.enabled = this.otherRenderer.hitType === renderTarget;
    }
}
