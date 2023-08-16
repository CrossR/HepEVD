//
// GUI Functions
//

import * as THREE from "three";

import { BUTTON_ID, TO_THEME } from "./constants.js";

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
  const dropDown = document.getElementById(`${hitDim}_dropdown`);
  const entries = new Set();

  // Add the default "None" option.
  entries.add(BUTTON_ID.None);

  if (hitPropMap.size != 0) entries.add(BUTTON_ID.All);

  hitPropMap.forEach((properties, _) => {
    properties.forEach((_, propString) => entries.add(propString));
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
  const dropDownButton = document.getElementById(`${hitDim}_dropdown_button`);
  dropDownButton.addEventListener("click", () => onClick(""));

  return;
}

/**
 * Populates a class toggle section with buttons based on the given hits array.
 * Adds a button for each unique class in the hits array.
 *
 * @param {string} hitDim - The dimension of the hit to which the toggle section belongs.
 * @param {Array} hits - An array of hit objects.
 * @param {function} onClick - The function to be called when a button is clicked.
 */
export function populateTypeToggle(hitDim, hits, onClick = (_) => {}) {
  const classDiv = document.getElementById(`types_${hitDim}`);
  const entries = new Set();

  hits.forEach((hit, _) => entries.add(hit.position.hitType));

  // If there is no entries, or only the default "Hit" class, don't bother.
  if (entries.size <= 1) {
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
export function populateMarkerToggle(hitDim, markers, onClick = (_) => {}) {
  const classDiv = document.getElementById(`markers_${hitDim}`);
  const entries = new Set();

  // TODO: Could potentially be extended, to use labels etc.
  markers.forEach((marker) => entries.add(marker.markerType));

  // If there is no entries, don't bother.
  if (entries.size < 1) {
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
    newButton.id = `markers_${entry}`;
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
  const classDiv = document.getElementById(`types_MC_${hitType}`);

  if (mcHits.length === 0) {
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
 * Updates the UI by toggling the visibility of the toggle options for the given class name.
 *
 * @param {string} hitType - The name of the hit class for which to toggle the visibility of the toggle options.
 */
export function updateUI(hitType) {
  const toggleOptions = document.getElementById("all_toggle_options");
  Array.from(toggleOptions.childNodes)
    .filter((elem) => elem.nodeName != "#text")
    .forEach((elem) => {
      // Toggle visibility for the new class.
      if (elem.id.includes(hitType)) {
        elem.style.visibility = "visible";
      } else {
        elem.style.visibility = "hidden";
      }
    });
}

/**
 * Saves a screenshot of the given renderer as a JPEG image and opens it in a new tab.
 *
 * @param {THREE.WebGLRenderer} renderer - The renderer to take a screenshot of.
 */
export function saveEvd(renderer) {
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
 */
export function quitEvd() {
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
        setTimeout(() => fadeOut(element, outDuration), 1500);
      }
    })();
  };

  const quittingElem = document.getElementById("quit_message");
  fadeInThenOut(quittingElem, 250, 550);

  // Actually perform the quit, now that the timers are running.
  fetch("quit");
}

/**
 * Swap the scene background colours to match the current theme.
 *
 * @param {Array} states - The states to animate.
 */
export function setTheme(states) {
  const themeName = localStorage.getItem("theme");

  // This occurs too quickly for the local storage to be correct.
  // So instead of setting it to the current value, invert the current value.
  const backgroundColor = TO_THEME[themeName];

  states.forEach((state) => {
    state.scene.background = new THREE.Color(backgroundColor);
    state.triggerEvent("change");
  });
}
