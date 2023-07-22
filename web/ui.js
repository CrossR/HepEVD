//
// GUI Functions
//

import { BUTTON_ID } from "./constants.js";

/**
 * Populates a dropdown menu with buttons based on the given hit property map.
 * Adds a "None" option by default, and an "All" option if the hit property map is not empty.
 * Finally, add a toggle for that scene on the dropdown itself.
 *
 * @param {string} className - The name of the class to which the dropdown belongs.
 * @param {Map} hitPropMap - A map of hit properties.
 * @param {function} onClick - The function to be called when a button is clicked.
 */
export function populateDropdown(className, hitPropMap, onClick = (_) => {}) {
  const dropDown = document.getElementById(`${className}_dropdown`);
  const entries = new Set();

  // Add the default "None" option.
  entries.add(BUTTON_ID.None);

  if (hitPropMap.size != 0) entries.add(BUTTON_ID.All);

  hitPropMap.forEach((properties, _) => {
    properties.forEach((_, propString) => entries.add(propString));
  });

  entries.forEach((entry) => {
    const newButton = document.createElement("button");
    newButton.innerText = entry;
    newButton.id = `${className}_${entry}`;
    newButton.addEventListener("click", () => onClick(entry));
    dropDown.appendChild(newButton);
  });

  // Add dropdown on click to send empty string.
  dropDown.parentElement.addEventListener("click", () => onClick(""));

  return;
}

/**
 * Populates a class toggle section with buttons based on the given hits array.
 * Adds a button for each unique class in the hits array.
 *
 * @param {string} className - The name of the class to which the toggle section belongs.
 * @param {Array} hits - An array of hit objects.
 * @param {function} onClick - The function to be called when a button is clicked.
 */
export function populateClassToggle(className, hits, onClick = (_) => {}) {
  const classDiv = document.getElementById(`classes_${className}`);
  const entries = new Set();

  hits.forEach((hit, _) => entries.add(hit.class));

  // If there is no entries, or only the default "Hit" class, don't bother.
  if (entries.size <= 1) {
    return;
  }

  entries.forEach((entry) => {
    const newButton = document.createElement("button");
    newButton.innerText = entry;
    newButton.id = `classes_${entry}`;
    newButton.addEventListener("click", () => onClick(entry));
    classDiv.appendChild(newButton);
  });

  return;
}

export function enableMCToggle(hitType, mcHits, onClick) {
  const classDiv = document.getElementById(`classes_MC_${hitType}`);

  if (mcHits.length === 0) {
    return;
  }

  const newButton = document.createElement("button");
  newButton.innerText = "MC Hits";
  newButton.id = `classes_MC_toggle_${hitType}`;
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
 * @param {string} className - The name of the class to which the dropdown or toggle section belongs.
 * @param {string} ID - The ID of the button to toggle.
 * @param {boolean} fixNoneButton - Whether or not to fix the state of the "None" button in the dropdown menu. Defaults to true.
 */
export function toggleButton(className, ID, fixNoneButton = true) {
  const button = document.getElementById(`${className}_${ID}`);

  if (button === null) return;

  let isActive = button.style.color === "white";

  if (isActive) {
    button.style.color = "green";
    isActive = false;
  } else {
    button.style.color = "white";
    isActive = true;
  }

  if (!fixNoneButton) return;

  if (ID === BUTTON_ID.None && isActive) {
    const dropDown = document.getElementById(`${className}_dropdown`);

    Array.from(dropDown.childNodes)
      .filter(
        (elem) =>
          elem.nodeName != "#text" &&
          elem != button &&
          elem.tagName.toLowerCase() === "button",
      )
      .forEach((elem) => {
        elem.style.color = "green";
      });
  } else if (ID !== BUTTON_ID.None && isActive) {
    const button = document.getElementById(`${className}_${BUTTON_ID.None}`);
    button.style.color = "green";
  }
}

/**
 * Determines whether a button with the given ID in the dropdown menu or class
 * toggle section with the given class name is currently active.
 *
 * @param {string} className - The name of the class to which the dropdown or toggle section belongs.
 * @param {string} ID - The ID of the button to check.
 * @returns {boolean} - True if the button is active (white), false otherwise.
 */
export function isButtonActive(className, ID) {
  const button = document.getElementById(`${className}_${ID}`);
  return button.style.color === "white";
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
        elem.style.display = "block";
      } else {
        elem.style.display = "none";
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
 * Quit the event display, allowing the server itself to close.
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
        element.style.display = "block";

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
  fadeInThenOut(quittingElem, 500, 750);

  // Actually perform the quit, now that the timers are running.
  fetch("quit");
}
