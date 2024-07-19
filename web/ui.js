//
// GUI Functions
//

import * as THREE from "three";

import { COLOUR_MAPS, DEFAULT_MAPS } from "./colourmaps.js";
import { BUTTON_ID, GITHUB_URL, TO_THEME } from "./constants.js";
import { createParticleMenu } from "./particle_menu.js";
import { renderImage } from "./rendering.js";
import { updateStateUI, reloadDataForCurrentState } from "./states.js";

/**
 * Populates a dropdown menu with buttons based on the given hit property map.
 * Adds a "None" option by default, and an "All" option if the hit property map is not empty.
 * Finally, add a toggle for that scene on the dropdown itself.
 *
 * @param {string} hitDim - The dimension of the hit to which the dropdown belongs.
 * @param {Map} hitPropMap - A map of hit properties.
 * @param {function} onClick - The function to be called when a button is clicked.
 */
export function populateDropdown(hitDim, hitPropMap, onClick = (_) => {}) {
  // Get the div to populate, and clear it to start.
  const dropDown = document.getElementById(`${hitDim}_dropdown`);
  dropDown.innerHTML = "";

  const entries = new Set();

  // Add the default "None" option.
  entries.add(BUTTON_ID.None);

  if (hitPropMap.size != 0) entries.add(BUTTON_ID.All);

  hitPropMap.forEach((properties, _) => {
    properties.forEach((_, propString) => {
      if (BUTTON_ID.Ignored.includes(propString)) return;
      entries.add(propString);
    });
  });

  entries.forEach((entry) => {
    const listElement = document.createElement("li");
    const newButton = document.createElement("li");
    newButton.style.textTransform = "capitalize";
    newButton.innerText = entry;
    newButton.id = `${hitDim}_${entry}`;
    newButton.addEventListener("click", () => onClick(entry));
    listElement.appendChild(newButton);
    dropDown.appendChild(listElement);
  });

  // Add dropdown on click to send empty string.
  // Clone the button to remove any other event listeners.
  if (!isTouchDevice()) {
    const dropDownButton = document.getElementById(`${hitDim}_dropdown_button`);
    dropDownButton.addEventListener("click", () => onClick(""));
  }

  return;
}

/**
 * Populates a class toggle section with buttons based on the given hits array.
 * Adds a button for each unique class in the hits array.
 *
 * @param {string} hitDim - The dimension of the hit to which the toggle section belongs.
 * @param {Map} hitTypeMap - A map of hit types.
 * @param {function} onClick - The function to be called when a button is clicked.
 */
export function populateTypeToggle(hitDim, hitTypesMap, onClick = (_) => {}) {
  // Get the div to populate, and clear it to start.
  const classDiv = document.getElementById(`types_${hitDim}`);
  classDiv.innerHTML = "";
  const entries = new Set();

  // Add a button for no hits.
  entries.add(BUTTON_ID.None);

  hitTypesMap.forEach((_, hitTypeString) => {
    if (hitTypeString === "All") return;
    entries.add(hitTypeString);
  });

  if (entries.size < 2) {
    classDiv.style.visibility = "hidden";
    return;
  }

  entries.forEach((entry) => {
    const newButton = document.createElement("button");
    newButton.classList.add(
      "btn",
      "btn-outline",
      "btn-accent",
      "m-1",
      "nohover",
    );
    newButton.style.textTransform = "capitalize";
    newButton.innerText = entry;
    newButton.id = `types_${entry}`;
    newButton.addEventListener("click", () => onClick(entry));
    classDiv.appendChild(newButton);
  });

  return;
}

/**
 * Populates the marker toggle section with buttons based on the given markers.
 *
 * @param {string} hitDim - The dimension of the hit to which the toggle section belongs.
 * @param {Array} markers - An array of marker objects.
 * @param {function} onClick - The function to be called when a button is clicked.
 */
export function populateMarkerToggle(
  hitDim,
  markers,
  particles,
  onClick = (_) => {},
) {
  // Get the div to populate, and clear it to start.
  const classDiv = document.getElementById(`markers_${hitDim}`);
  classDiv.innerHTML = "";

  const entries = new Set();

  // TODO: Could potentially be extended, to use labels etc.
  markers.forEach((marker) => entries.add(marker.markerType));

  // If there are vertices anywhere in the particle array, add a vertex toggle.
  if (particles.some((particle) => particle.vertices.length > 0)) {
    entries.add("Point");
  }

  // If there is no entries, don't bother.
  if (entries.size < 1) {
    classDiv.style.visibility = "hidden";
    return;
  }

  entries.forEach((entry) => {
    const newButton = document.createElement("button");
    newButton.classList.add(
      "btn",
      "btn-outline",
      "btn-accent",
      "m-1",
      "nohover",
    );
    newButton.style.textTransform = "capitalize";
    newButton.innerText = entry;
    newButton.id = `markers_${hitDim}_${entry}`;
    newButton.addEventListener("click", () => onClick(entry));
    classDiv.appendChild(newButton);
  });

  return;
}

/**
 * Enables a toggle button for MC hits in the class toggle section with the given hit type.
 * If there are no MC hits, the function does nothing.
 *
 * @param {string} hitType - The hit type for which to enable the MC toggle button.
 * @param {Array} mcHits - An array of MC hit objects.
 * @param {function} onClick - The function to be called when the MC toggle button is clicked.
 */
export function enableMCToggle(hitType, mcHits, onClick) {
  // Get the div to populate, and clear it to start.
  const classDiv = document.getElementById(`types_MC_${hitType}`);
  classDiv.innerHTML = "";

  if (mcHits.length === 0) {
    classDiv.style.visibility = "hidden";
    return;
  }

  const newButton = document.createElement("button");
  newButton.classList.add("btn", "btn-outline", "btn-accent", "m-1", "nohover");
  newButton.innerText = "MC Hits";
  newButton.id = `types_MC_toggle_${hitType}`;
  newButton.addEventListener("click", () => onClick());
  classDiv.appendChild(newButton);

  return;
}

/**
 * Enables a toggle button for different particle interaction types.
 * If there are no particles, the function does nothing.
 *
 * @param {string} hitType - The hit type for which to enable the particle toggle button.
 * @param {Array} particles - An array of particle objects.
 * @param {function} onClick - The function to be called when the particle toggle button is clicked.
 */
export function enableInteractionTypeToggle(hitType, particles, onClick) {
  const classDiv = document.getElementById(`particles_${hitType}`);
  classDiv.innerHTML = "";

  if (particles.length === 0) {
    classDiv.style.visibility = "hidden";
    return;
  }

  const interactionTypes = new Set();
  particles.forEach((particle) => {
    interactionTypes.add(particle.interactionType);
  });

  if (interactionTypes.size < 2) {
    classDiv.style.visibility = "hidden";
    return;
  }

  interactionTypes.forEach((interactionType) => {
    const newButton = document.createElement("button");
    newButton.classList.add(
      "btn",
      "btn-outline",
      "btn-accent",
      "m-1",
      "nohover",
    );
    newButton.innerText = interactionType;
    newButton.id = `particles_${hitType}_${interactionType}`;
    newButton.addEventListener("click", () => onClick(interactionType));
    classDiv.appendChild(newButton);
  });

  return;
}

/**
 * Toggles the active state of a button with the given ID in the dropdown menu
 * or class toggle section with the given class name. If the button is the
 * "None" button, it also toggles the state of every other button in that
 * dropdown.
 *
 * @param {string} hitDim - The dimension of the hit to which the dropdown belongs.
 * @param {string} ID - The ID of the button to toggle.
 * @param {boolean} fixNoneButton - Whether or not to fix the state of the "None" button in the dropdown menu. Defaults to true.
 */
export function toggleButton(hitDim, ID, fixNoneButton = true) {
  const button = document.getElementById(`${hitDim}_${ID}`);

  if (button === null) return;

  let isActive = button.classList.contains("btn-active");

  if (isActive) {
    button.classList.remove("btn-active");
    isActive = false;
  } else {
    button.classList.add("btn-active");
    isActive = true;
  }

  if (!fixNoneButton) return;

  if (ID === BUTTON_ID.None && isActive) {
    const dropDown = document.getElementById(`${hitDim}_dropdown`);

    Array.from(dropDown.childNodes).forEach((elem) => {
      elem.childNodes[0].classList.remove("btn-active");
    });
  } else if (ID !== BUTTON_ID.None && isActive) {
    const noneButton = document.getElementById(`${hitDim}_${BUTTON_ID.None}`);
    noneButton.classList.remove("btn-active");
  }
}

/**
 * Determines whether a button with the given ID in the dropdown menu or class
 * toggle section with the given class name is currently active.
 *
 * @param {string} hitDim - The dimension of the hit to which the dropdown belongs.
 * @param {string} ID - The ID of the button to check.
 * @returns {boolean} - True if the button is active, false otherwise.
 */
export function isButtonActive(hitDim, ID) {
  const button = document.getElementById(`${hitDim}_${ID}`);
  return button ? button.classList.contains("btn-active") : false;
}

/**
 * Sets up the particle menu with the given render state.
 *
 * @param {Object} renderState - The render state object.
 * @returns {void}
 */
export function setupParticleMenu(renderState) {
  const onClickAction = (particle) => {
    const particleID = particle.id;
    const particleMenuEntry = document.getElementById(
      `particle_${particleID}_${renderState.hitDim}`,
    );
    const label = particleMenuEntry.querySelector("span");

    if (renderState.particleData.checkIgnored(particle)) {
      renderState.particleData.unignoreParticle(particle);
      label.classList.remove("line-through");
    } else {
      renderState.particleData.ignoreParticle(particle);
      label.classList.add("line-through");
    }

    renderState.renderEvent();
  };

  createParticleMenu(
    renderState.hitDim,
    renderState.particleData.particleMap,
    onClickAction,
  );
}

/**
 * Toggles the visibility of the particle menu based on the active hit type.
 *
 * @param {string} activeHitType - The active hit type, either "2D" or "3D".
 */
export function toggleVisibleParticleMenu(activeHitType) {
  const particleMenu2D = document.getElementById("particle_menu_2D");
  const particleMenu3D = document.getElementById("particle_menu_3D");

  if (activeHitType === "2D") {
    particleMenu2D.style.display = "block";
    particleMenu3D.style.display = "none";
  } else {
    particleMenu2D.style.display = "none";
    particleMenu3D.style.display = "block";
  }
}

/**
 * Toggles the visibility of the particle menu based on the active hit type.
 *
 * @param {string} mcTruthString - The current MC truth string.
 */
export function setMCTruth(mcTruthString) {
  const mcTruthElem = document.getElementById("mc_truth");

  if (mcTruthString === "") {
    mcTruthElem.style.display = "none";
    return;
  }

  katex.render(mcTruthString, mcTruthElem, {
    throwOnError: false,
  });
  mcTruthElem.style.display = "block";
}

/**
 * Updates the UI by toggling the visibility of various menu options.
 *
 * @param {string} activeHitType - The name of the hit class for which to toggle the visibility of the toggle options.
 * @param {string} mcTruthString - The current MC truth string.
 */
export function updateUI(activeHitType, mcTruthString) {
  const toggleOptions = document.getElementById("all_toggle_options");
  Array.from(toggleOptions.childNodes)
    .filter((elem) => elem.nodeName != "#text")
    .forEach((elem) => {
      // Toggle visibility for the new class.
      if (elem.id.includes(activeHitType)) {
        elem.style.visibility = "visible";
      } else {
        elem.style.visibility = "hidden";
      }
    });
  toggleVisibleParticleMenu(activeHitType);
  setMCTruth(mcTruthString);
}

/**
 * Saves a screenshot of the given renderer as a JPEG image and opens it in a new tab.
 *
 * @param {THREE.WebGLRenderer} renderer - The renderer to take a screenshot of.
 */
export function screenshotEvd(renderer) {
  const imageData = renderer.domElement.toDataURL("image/jpeg", 1.0);
  const contentType = "image/jpeg";

  const byteCharacters = atob(
    imageData.substr(`data:${contentType};base64,`.length),
  );
  const bytes = [];

  for (let offset = 0; offset < byteCharacters.length; offset += 1024) {
    const slice = byteCharacters.slice(offset, offset + 1024);
    const byteNumbers = new Array(slice.length);
    for (let i = 0; i < slice.length; i++) {
      byteNumbers[i] = slice.charCodeAt(i);
    }

    const byteArray = new Uint8Array(byteNumbers);
    bytes.push(byteArray);
  }

  const blob = new Blob(bytes, { type: contentType });
  const blobUrl = URL.createObjectURL(blob);

  window.open(blobUrl, "_blank");
}

/**
 * Sends a request to the server to quit the event display.
 *
 * @param {Map} renderStates - The states to save.
 */
export async function quitEvd(renderStates) {
  const fadeOut = (element, duration) => {
    (function decrement() {
      (element.style.opacity -= 0.1) < 0
        ? (element.style.display = "none")
        : setTimeout(() => {
            decrement();
          }, duration / 10);
    })();
  };
  const fadeInThenOut = (element, inDuration, outDuration) => {
    (function increment(value = 0) {
      element.style.opacity = String(value);

      if (window.getComputedStyle(element, null).display === "none")
        element.style.display = "grid";

      if (element.style.opacity !== "1") {
        setTimeout(() => {
          increment(value + 0.1);
        }, inDuration / 10);
      } else {
        setTimeout(() => fadeOut(element, outDuration), 500);
      }
    })();
  };

  const quittingElem = document.getElementById("quit_message");
  fadeInThenOut(quittingElem, 150, 150);

  // Get a copy of the current event state...
  const currentState = await fetch("stateInfo").then((response) =>
    response.json(),
  );

  // Actually perform the quit, now that the timers are running.
  fetch("quit");

  // Now, on an interval timer, keep checking the current state to see if it has
  // changed. If it has, then reload the data from the server.
  // First, declare a comparison function for the two states.
  const compareStates = (a, b) => {
    return JSON.stringify(a) === JSON.stringify(b);
  };

  // Keep track of the number of times we've tried to reload the data.
  let reloadCount = 0;
  let reloadInterval = 250;

  const reloadFunction = setInterval(async () => {
    // If we've tried for 30s, then give up.
    if (reloadCount * reloadInterval > 30000) {
      clearInterval(reloadFunction);
      return;
    }

    try {
      const newState = await fetch("stateInfo").then((response) => {
        if (response.ok) return response.json();
      });

      reloadCount += 1;

      if (compareStates(newState, currentState)) return;

      console.log("Reloading data...");

      updateStateUI(renderStates);
      reloadDataForCurrentState(renderStates);

      clearInterval(reloadFunction);
    } catch (_) {}
  }, reloadInterval);
}

/**
 * Swap the scene background colours to match the current theme.
 *
 * @param {Map} states - The states to animate.
 */
export function toggleTheme(states) {
  const themeName = localStorage.getItem("themeInfo");

  // This occurs too quickly for the local storage to be correct.
  // So instead of setting it to the current value, invert the current value.
  const backgroundColor = TO_THEME[themeName];

  // Set the new theme in storage, so that it is correct for the next time.
  const newTheme = themeName === "light" ? "dark" : "light";
  localStorage.setItem("themeInfo", newTheme);

  states.forEach((state) => {
    state.scene.background = new THREE.Color(backgroundColor);
    state.triggerEvent("change");
  });

  fixThemeButton();
}

/**
 * Simply fix the theme button, by setting the text and emoji to the inverse of
 * the current theme.
 */
export function fixThemeButton() {
  let themeName = localStorage.getItem("themeInfo");

  // If there is nothing, just set it to dark.
  if (themeName === null) {
    themeName = "dark";
    localStorage.setItem("themeInfo", themeName);
  }

  // These are inverted, since we want to change to the opposite theme.
  const emojis = { dark: "☀️", light: "🌙" };
  const themeButton = document.getElementById("theme_button");
  themeButton.innerHTML = `${emojis[themeName]} Change Theme`;
}

/**
 * Save the current state of the event display to local storage.
 *
 * @param {Map} states - The states to save.
 * @param {String} name - The name of the state to save.
 */
export function saveState(states) {
  const inputModal = document.getElementById("input_modal");
  const inputSave = document.getElementById("input_modal_save");
  const inputText = document.getElementById("input_modal_input");
  const inputBackdrop = document.getElementById("input_backdrop");
  const inputClose = document.getElementById("input_close");

  inputText.placeholder = "Enter a name for this state";

  inputModal.showModal();

  let closed = false;
  const cleanUp = () => {
    inputText.placeholder = "";
    inputText.value = "";
  };
  const doSave = () => {
    const name = inputText.value;

    if (name === undefined || name === "") return;
    const visibleState = Array.from(states.values()).find(
      (state) => state.visible,
    );
    const store = window.localStorage;

    const cameraPos = visibleState.camera.position;
    const cameraUp = visibleState.camera.up;
    const cameraTarget = visibleState.controls.target;

    const state = {
      name: name,
      hitDim: visibleState.hitDim,
      camera: {
        fov: visibleState.camera.fov,
        near: visibleState.camera.near,
        far: visibleState.camera.far,
        masks: visibleState.camera.layers.mask,
        position: [cameraPos.x, cameraPos.y, cameraPos.z],
        up: [cameraUp.x, cameraUp.y, cameraUp.z],
      },
      controls: {
        target: [cameraTarget.x, cameraTarget.y, cameraTarget.z],
      },
    };

    let saveStates = [state];

    if (store.getItem("saveStates") !== null) {
      saveStates = JSON.parse(store.getItem("saveStates"));
      saveStates.push(state);
    }

    store.setItem("saveStates", JSON.stringify(saveStates));
  };

  [inputBackdrop, inputClose].forEach((elem) =>
    elem.addEventListener(
      "click",
      () => {
        closed = true;
        cleanUp();
      },
      { once: true },
    ),
  );

  inputSave.addEventListener(
    "click",
    () => {
      if (closed) return;

      doSave();
      cleanUp();
    },
    { once: true },
  );
}

/**
 * Load the given state from local storage.
 *
 * @param {Map} renderStates - The states to save.
 */
export function loadState(renderStates) {
  const visibleState = Array.from(renderStates.values()).find(
    (state) => state.visible,
  );
  const store = window.localStorage;

  const selectModal = document.getElementById("select_modal");
  const selectButton = document.getElementById("select_modal_choose");
  const selectDropdown = document.getElementById("select_modal_options");
  const selectBackdrop = document.getElementById("select_backdrop");
  const selectClose = document.getElementById("select_close");

  // Parse out the saved states and then filter to ones for the current view.
  // If there isn't any, return with no action.
  const saveStates = JSON.parse(store.getItem("saveStates"));

  if (saveStates === null) {
    return;
  }

  const validSaveStates = saveStates.filter(
    (state) => state.hitDim === visibleState.hitDim,
  );

  if (validSaveStates === null) return;

  // Since there are valid states, add them to the dropdown list.
  validSaveStates.forEach((state) => {
    const option = document.createElement("option");
    option.text = state.name;
    selectDropdown.add(option);
  });

  // Finally show the modal.
  selectModal.showModal();

  const cleanUp = () => {
    selectDropdown.innerHTML = "";
  };
  const doLoad = () => {
    // Update all the camera properties.
    const newState = validSaveStates[selectDropdown.selectedIndex];
    visibleState.camera.fov = newState.camera.fov;
    visibleState.camera.near = newState.camera.near;
    visibleState.camera.far = newState.camera.far;
    visibleState.camera.layers.mask = newState.camera.masks;
    visibleState.camera.position.set(...newState.camera.position);
    visibleState.camera.up.set(...newState.camera.up);
    visibleState.controls.target.set(...newState.controls.target);

    visibleState.camera.updateProjectionMatrix();
    visibleState.controls.update();
  };
  let closed = false;

  [selectBackdrop, selectClose].forEach((elem) =>
    selectBackdrop.addEventListener(
      "click",
      () => {
        closed = true;
        cleanUp();
      },
      { once: true },
    ),
  );

  selectButton.addEventListener(
    "click",
    () => {
      if (closed) return;
      doLoad();
      cleanUp();
      visibleState.triggerEvent("change");
    },
    { once: true },
  );
}

/**
 * Show a modal to pick the current colourscheme.
 */
export function pickColourscheme(states) {
  const visibleState = Array.from(states.values()).find(
    (state) => state.visible,
  );
  const store = window.localStorage;

  const selectModal = document.getElementById("select_modal");
  const selectButton = document.getElementById("select_modal_choose");
  const categoricalSelect = document.getElementById("select_modal_options");
  const selectBackdrop = document.getElementById("select_backdrop");
  const selectClose = document.getElementById("select_close");

  // First, we need to duplicate the select, since we need two, one
  // for categorical and one for continuous.
  const continuousSelect = categoricalSelect.cloneNode(true);
  categoricalSelect.parentElement.appendChild(continuousSelect);

  // Add a default, unselected option with the placeholder text.
  [
    [categoricalSelect, "Categorical"],
    [continuousSelect, "Continuous"],
  ].forEach(([select, placeholder]) => {
    const option = document.createElement("option");
    option.text = placeholder;
    option.disabled = true;
    option.selected = true;
    select.add(option);
  });

  // Then, add all the options to the dropdown.
  [...Object.keys(COLOUR_MAPS), ...Object.keys(DEFAULT_MAPS)].forEach(
    (csName) => {
      const option = document.createElement("option");
      option.text = csName;
      categoricalSelect.add(option);
      continuousSelect.add(option.cloneNode(true));
    },
  );

  // Finally show the modal.
  selectModal.showModal();

  const cleanUp = () => {
    categoricalSelect.innerHTML = "";
    continuousSelect.innerHTML = "";

    // We also need to remove the continuous select.
    continuousSelect.parentElement.removeChild(continuousSelect);
  };
  const doSave = () => {
    if (categoricalSelect.selectedIndex !== 0) {
      const map =
        COLOUR_MAPS[categoricalSelect.value] ||
        DEFAULT_MAPS[categoricalSelect.value];

      const result = JSON.stringify({
        name: categoricalSelect.value,
        size: map.length ?? map,
      });

      store.setItem("categoricalColourMap", result);
    }

    if (continuousSelect.selectedIndex !== 0) {
      const map =
        COLOUR_MAPS[continuousSelect.value] ||
        DEFAULT_MAPS[continuousSelect.value];

      const result = JSON.stringify({
        name: continuousSelect.value,
        size: map.length ?? map,
      });

      store.setItem("continuousColourMap", result);
    }
  };
  let closed = false;

  [selectBackdrop, selectClose].forEach((elem) =>
    elem.addEventListener(
      "click",
      () => {
        closed = true;
        cleanUp();
      },
      { once: true },
    ),
  );

  selectButton.addEventListener(
    "click",
    () => {
      if (closed) return;
      doSave();
      cleanUp();

      visibleState.triggerEvent("fullUpdate");
    },
    { once: true },
  );
}

/**
 * Makes an element draggable.
 *
 * @param {HTMLElement} element - The element to make draggable.
 */
export function dragElement(element) {
  let startX = 0,
    startY = 0,
    endX = 0,
    endY = 0;
  element.onmousedown = dragMouseDown;

  function dragMouseDown(e) {
    e = e || window.event;
    e.preventDefault();
    startX = e.clientX;
    startY = e.clientY;
    document.onmouseup = closeDragElement;
    document.onmousemove = elementDrag;
  }

  function elementDrag(e) {
    e = e || window.event;
    e.preventDefault();
    endX = startX - e.clientX;
    endY = startY - e.clientY;
    startX = e.clientX;
    startY = e.clientY;
    element.style.top = element.offsetTop - endY + "px";
    element.style.left = element.offsetLeft - endX + "px";
  }

  function closeDragElement() {
    document.onmouseup = null;
    document.onmousemove = null;
  }
}

/**
 * Adds a citation to the document.
 *
 * @param {string} citationStr - The citation string to be added.
 * @param {string} citationURL - The URL to the citation.
 */
export function addCitation(citationStr, citationURL) {
  const citationDiv = document.getElementById("citations");

  // First, add the citation text, with a link to the URL.
  const citation = document.createElement("p");
  citation.innerHTML = `<a href="${citationURL}">${citationStr}</a>`;
  citationDiv.appendChild(citation);

  // Plus a personal citation, linking back to the GitHub.
  const personalCitation = document.createElement("p");
  personalCitation.innerHTML = `<a href="${GITHUB_URL}" target="_blank">Event display developed by Ryan Cross</a>`;
  personalCitation.style.fontSize = "0.8em";

  citationDiv.appendChild(personalCitation);

  return;
}

/**
 * Is this a touchscreen device?
 *
 * @returns {boolean}
 */
export function isTouchDevice() {
  return window.matchMedia("(pointer: coarse)").matches;
}

/**
 * Setup mobile / touchscreen UI, by updating some of the buttons.
 *
 * @param {THREE.WebGLRenderer} Renderer
 */
export function setupMobileUI(renderer) {
  // Is the device low horizontal resolution? i.e. less then 640?
  const shouldResize = window.innerWidth < 640;

  // Does the device primarily use a touchscreen?
  const isTouch = isTouchDevice();

  if (!shouldResize && !isTouch) return;

  // First, sort out the touch devices, as that should cover mobiles too.
  // We want to swap every button to instead be dropdown on tap, not hover.
  let buttons = [
    document.getElementById("state_dropdown_button"),
    document.getElementById("2D_dropdown_button"),
    document.getElementById("3D_dropdown_button"),
  ];
  const dropDowns = buttons.map((button) => button.nextElementSibling);

  // When tapped, toggle the hover state.
  buttons.forEach((button) => {
    // Remove any existing onclick handlers.
    button.removeEventListener("click", () => {});
    button.onclick = null;

    // Find the dropdown element via the parent.
    const dropdownElem = button.nextElementSibling;
    button.addEventListener("click", () => {
      const visibility = dropdownElem.style.visibility;
      if (visibility === "hidden" || visibility === "") {
        dropdownElem.style.visibility = "visible";
        dropdownElem.style.opacity = "1";
      } else {
        dropdownElem.style.visibility = "hidden";
        dropdownElem.style.opacity = "0";
      }
    });
  });

  // Next, setup the actual rendering canvas to hide the above dropdowns on
  // interaction.
  renderer.domElement.addEventListener("touchstart", (event) => {
    dropDowns.forEach((dropDown) => {
      dropDown.style.visibility = "hidden";
      dropDown.style.opacity = "0";
    });
  });

  if (!shouldResize) return;

  // Now, let's hide unnecessary buttons.
  buttons = [
    document.getElementById("previous_state"),
    document.getElementById("next_state"),
  ];
  buttons.forEach((button) => {
    button.style.display = "none";
  });
  const options = document.getElementById("options_button");
  options.style.display = "none";
  options.nextElementSibling.style.display = "none";

  // Next, lets update the names for the buttons...
  const names = {
    quit_button: "🛑",
    options_button: "⚙️",
    "2D_dropdown_button": "2D",
    "3D_dropdown_button": "3D",
  };

  Object.keys(names).forEach((key) => {
    const elem = document.getElementById(key);
    elem.innerHTML = names[key];
  });
}

/**
 * Populates the impage drop down, to toggle showing of an image.
 *
 * @param {Array} images - The images to populate the dropdown with.
 */
export function populateImages(images) {
  const dropDownButton = document.getElementById(`image_dropdown_button`);
  const dropDown = document.getElementById(`image_dropdown`);
  dropDown.innerHTML = "";

  if (!images || images.length === 0) {
    dropDownButton.style.display = "none";
    return;
  } else {
    dropDownButton.style.display = "grid";
  }

  const showImage = (rawImage, imageElem) => {
    // Create a div to hold the image...
    const imDiv = document.createElement("div");
    imDiv.classList.add("evd_image", "text-right");
    dragElement(imDiv);

    // Attach a close button...
    const closeBtn = document.createElement("label");
    closeBtn.classList.add("btn", "btn-error", "m-1", "fixed");
    closeBtn.style.marginLeft = "-40px";
    closeBtn.style.bottom = "101%";
    closeBtn.innerHTML = "X";
    closeBtn.addEventListener("click", () => {
      imDiv.remove();
    });

    // And the image name
    const label = document.createElement("label");
    label.style.position = "fixed";
    label.style.left = "-10px";
    label.style.top = "98%";
    label.innerHTML = rawImage.label;
    label.style.margin = "0.5em";

    // Add the div to the page.
    imDiv.appendChild(label);
    imDiv.appendChild(closeBtn);
    imDiv.appendChild(imageElem.cloneNode());
    document.body.appendChild(imDiv);
  };

  images.forEach((image) => {
    const listElement = document.createElement("li");
    const newButton = document.createElement("li");
    newButton.style.textTransform = "capitalize";
    newButton.innerText = image.label;
    newButton.id = `${image.label}`;
    const im = renderImage(image);
    newButton.addEventListener("click", () => showImage(image, im));
    listElement.appendChild(newButton);
    dropDown.appendChild(listElement);
  });

  return;
}
