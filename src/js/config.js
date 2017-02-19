module.exports = [
  {
    "type": "heading",
    "defaultValue": "Easy Time Configuration"
  },
  {
    "type": "text",
    "defaultValue": "Change the look and feel of your Easy Time watchface."
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Colors"
      },
      {
        "type": "color",
        "messageKey": "BackgroundColor",
        "defaultValue": "0xffffff",
        "label": "Background Color"
      },
      {
        "type": "color",
        "messageKey": "PrimaryColor",
        "defaultValue": "0x0026ff",
        "label": "Primary Color"
      },
      {
        "type": "color",
        "messageKey": "SecondaryColor",
        "defaultValue": "0xb4b4b4",
        "label": "Secondary Color"
      }
    ]
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "More Settings"
      },
      {
        "type": "toggle",
        "messageKey": "UseDayCounter",
        "label": "Day countdown",
        "defaultValue": "true"
      },
      {
        "type": "input",
        "messageKey": "DayCounterDate",
        "label": "Select date",
        "attributes": {
            "type": "date"
          }        
      },      
      {
        "type": "toggle",
        "messageKey": "ShowBattery",
        "label": "Show battery percentage",
        "defaultValue": true
      }
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save Settings"
  }
];