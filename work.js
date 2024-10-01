// Helper function to recursively search for a string in child elements
function searchForTextInChildren(element, text) {
  // console.log(`Searching in element: ${element.outerHTML}`); // Log element for debugging
  if (element.tagName === 'SPAN' && element.textContent.trim() === text) {
    return element;
  }
  for (let child of element.children) {
    const found = searchForTextInChildren(child, text);
    if (found) {
      return found;
    }
  }
  return null;
}

// Function to wait for an element or multiple elements to appear
function waitForElements(selector) {
  return new Promise((resolve, reject) => {
    const elements = document.querySelectorAll(selector);
    if (elements.length > 0) {
      resolve(elements);
    } else {
      const observer = new MutationObserver((mutations) => {
        const newElements = document.querySelectorAll(selector);
        if (newElements.length > 0) {
          observer.disconnect();
          resolve(newElements);
        }
      });
      observer.observe(document.body, { childList: true, subtree: true });
    }
  });
}

// Function to introduce a delay between operations
function wait(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}

// Function to process the 'Manage' buttons workflow
async function processManageButtons() {
  try {
    const manageButtons = await waitForElements('.x1i10hfl[aria-label="Manage"]');
    // console.log(manageButtons);
    
    for (let button of manageButtons) {
      // await wait(500); // Add a 1-second delay before each click
      button.click(); // Click the 'Manage' button
      
      // Wait for the submenu items to appear
      const subelements = await waitForElements('[role="menuitem"]');
      // console.log(`Subelements found: ${subelements.length}`);

      let blockFound = false;

      for (let subelement of subelements) {
        try {
          const blockElement = searchForTextInChildren(subelement, 'Block');
          if (blockElement) {
            // console.log(`Block found: ${blockElement}`);
            await wait(500); // Add a 1-second delay before clicking 'Block'
            subelement.click(); // Click the 'Block' item
            blockFound = true;
            break; // Stop after finding and clicking 'Block'
          }
        } catch (error) {
          console.error('Error searching in submenu item:', error);
        }
      }

      if (!blockFound) {
        throw new Error('Block element not found in any submenu item');
      }

      // Wait for the confirm button to appear and click it
      const confirmButton = await waitForElements('[tabindex="0"][aria-label="Confirm"]');
      await wait(500); // Add a 1-second delay before clicking 'Confirm'
      // console.log(confirmButton);
      confirmButton[0].click();

      // Wait for the Close button to appear and click it
      const closeButton = await waitForElements('[aria-label="Close"]');
      // console.log(closeButton);
      closeButton[0].click();
    }

  } catch (error) {
    console.error('Error in workflow:', error);
  }
}

// Function to poll for new 'Manage' buttons at a constant interval
function startPolling(interval = 2000) {
  setInterval(() => {
    console.log('Polling for new Manage buttons...');
    processManageButtons();
  }, interval);
}

// Start polling every 5 seconds (5000ms)
startPolling(2000);
