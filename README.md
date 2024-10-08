# sc4-custom-budget-departments

A DLL plugin for SimCity 4 that allows new budget departments to be added to the game using a building's exemplar.

The budget department line item expense and/or income value(s) can be fixed/constant or variable based on several algorithms.

## Exemplar Properties

The DLL depends on the game's existing `Budget Item:` properties, and adds a few new properties. 
Each property supports multiple values.

### Property Tables

#### Main Budget Department Properties

| Property ID | Property Name | Type | Description   |
|-------------|---------------|------|--------------------------
| 0xEA54D283  | Budget Item: Department | Uint32 | Used to set the budget department id(s). For new departments it should be a random IID value. |
| 0xEA54D284  | Budget Item: Line | Uint32 | Used to set the budget department line items for the building. It should be a random IID value for each custom budget department line item. |
| 0xEA54D285  | Budget Item: Purpose | Uint32 | Set to `0x87BD3990` for an expense line item or `0x46261226` for an income line item. |
| 0xEA54D286  | Budget Item: Cost | Sint64 | Cost(s) of each line item. |
| 0x90222B81  | Budget: Custom Department Budget Group | Uint32 | Controls which budget window the custom budget department is grouped under. There must be one entry for each custom budget department. Each entry is a series of 2 Uint32 values, consisting of the department id followed by the budget group id. See the `Budget Groups` table below. |
| 0x4252085F  | Budget: Custom Department Name Key | Uint32 | Specifies the name key for a custom budget department. There must be one entry for each custom budget department. Each entry is a series of 3 Uint32 values, consisting of the department id followed by the group and instance ids of the LTEXT file. |
| 0x9EE1240F  | Budget: Custom Line Item Cost Algorithm | Uint32 | An optional property to configure the cost algorithm that is used for the line item(s). If it is used, there must be one entry for each custom budget line item. Each entry is a series of 2 UInt32 values, consisting of the line item id followed by the algorithm id. If the property is not present, the `Fixed` cost algorithm will be used. See the `Custom Line Item Cost Algorithm` table below. |

#### Budget Groups

| Budget Group ID | Budget Window |
|-----------------|-------------|
| 0x0A5A72D1 | Business Deals |
| 0x6A357B96 | City Beautification |
| 0xEA597195 | Government Buildings |
| 0x6A357B7F | Health and Education |
| 0x4A357B40 | Public Safety |
| 0xAA369059 | Transportation |
| 0x4A357EAF | Utilities |

####  Custom Line Item Cost Algorithm

| Custom Line Item Cost Algorithm ID | Name | Description |
|--------------------------|------|------|
| 0x00000000 | Fixed | The item has a fixed expense/income, as specified by the `Budget Item: Cost` property. |
| 0x00000001 | Variable City Residential Total Pop. | The fixed expense/income set by the `Budget Item: Cost` property will vary based a factor of the city's total residential population. Uses the `Budget Custom Line Item Variable Expense/Income: Res. Total Pop.` property. |
| 0x00000002 | Variable City Residential Wealth Groups Pop. | The fixed expense/income set by the `Budget Item: Cost` property will vary based factors of the city's residential population by wealth group. Uses the `Budget Custom Line Item Variable Expense/Income: Res. Wealth Group Pop.` property. |
| 0x00000003 | Variable Tourism | The fixed expense/income set by the `Budget Item: Cost` property will vary based factors related to an approximation of local/regional tourism. Uses the `Budget Custom Line Item Variable Expense/Income: Tourism`. |

#### Custom Line Item Cost Algorithm Tuning Properties

| Custom Line Item Cost Algorithm Tuning Property ID | Property Name | Type | Description |
|----------------------------------------------------|---------------|------|-------------|
| 0x9EE12410 | Budget Custom Line Item Variable Expense/Income: Res. Total Pop. | Sint64 | Factor applied to the budget item expense/income based on the total residential population. The format is a group of 3 Sint64 values representing the line item id followed by the numerator and denominator for the total residential population factor. |
| 0x9EE12411 | Budget Custom Line Item Variable Expense/Income: Res. Wealth Groups Pop. | Sint64 | Factor applied to the budget item expense/income based on the residential wealth group populations. The format is a group of 7 Sint64 values representing the line item id followed by the numerators and denominators for the low, medium, and high wealth group factors. |
| 0x9EE12412 | Budget Custom Line Item Variable Expense/Income: Tourism | Sint64 | Factor applied to the budget item expense/income based on an algorithm that approximates local/regional tourism. The format is a group of 4 Sint64 fields representing the line number id followed by a numerator and denominator for the national and international tourism factor and a Sint64 geopolitical factor. |

##### Tourism Algorithm Details

The tourism algorithm uses the city and regional residential populations to approximate local/regional tourism mechanic. The algorithm is described below:
```
x = Low Wealth Population City
y = Medium Wealth Population City
z = High Wealth Population City
j = Low Wealth Population Region
k = Medium Wealth Population Region
l = High Wealth Population Region
p = Geopolitics Factor
d = National & International Tourism factor

Variable Expense/Income = [x + y + z + (j * d) + (k * d) + (l * d)] / p
```

### Example Building Exemplar Properties

This example shows part of a building exemplar with a custom department that has both expense and income items.

| Property ID | Property Name | Type | Reps | Values | Value Description |
|--------------------------------|------------|--------|---|-----------------------|--------------------------------|  
| 0xEA54D283 | Budget Item: Department         | Uint32 | 2 | 0x790651AD,0x790651AD | The department id(s). Usually the same value, but it should be possible for the line items to belong to a different department.  |
| 0xEA54D284 | Budget Item: Line               | Uint32 | 2 | 0x6FB01C57,0x6FB01C58 | The line item id(s). Should be a random number that uniquely identifies the custom budget department line item. |
| 0xEA54D285 | Budget Item: Purpose            | Uint32 | 2 | 0x87BD3990,0x46261226 | The purpose id(s). In this case the  custom budget department expense id followed by the custom budget department income id.
| 0xEA54D286 | Budget Item: Cost               | Sint64 | 2 | 0x0000000000000064,0x00000000000000FA | The line item cost(s). In this case the expense line item followed by the income line item.
| 0x90222B81 | Budget: Custom Department Budget Group | Uint32 | 4 | 0x790651AD,0x0A5A72D1,0x790651AD,0x0A5A72D1 | The department id(s) and the budget group that id belongs to. In this case both items belong to the 'Business Deals' group. Reps must be a multiple of 2. |
| 0x4252085F | Budget: Custom Department Name Key     | Uint32 | 6 | 0x790651AD,0x26350A44,0x030F0A4F,0x790651AD,0x26350A44,0x030F0A4F | The department id(s) followed by the group and instance ids of the LTEXT file that contain the department name. Reps must be a multiple of 3. | 
| 0x9EE1240F | Budget: Custom Line Item Cost Algorithm | Uint32 | 4 | 0x6FB01C57,0x00000000,0x6FB01C58,0x00000001 | The line item id(s) followed by the line item algorithm id. In this case, a fixed cost expense item and a variable income item.  Reps must be a multiple of 2. |
| 0x9EE12410 | Budget Custom Line Item Variable Expense/Income: Res. Total Pop. | Sint64 | 3 | 0x000000006FB01C58,0x0000000000000002,0x00000000000003E8 | The factor that is applied to the budge item income/expense based on the city population. In this case, §2 in additional income for every 1,000 city residents.|


## System Requirements

* Windows 10 or later

The plugin may work on Windows 7 or later with the [Microsoft Visual C++ 2022 x86 Redistribute](https://aka.ms/vs/17/release/vc_redist.x86.exe) installed, but I do not have the ability to test that.

## Installation

1. Close SimCity 4.
2. Copy `CustomBudgetDepartments.dll` into the top-level of the Plugins folder in the SimCity 4 installation directory
or Documents/SimCity 4 directory. 
3. Start SimCity 4.

## Troubleshooting

The plugin should write a `CustomBudgetDepartments.log` file in the same folder as the plugin.    
The log contains status information for the most recent run of the plugin.

# License

This project is licensed under the terms of the MIT License.    
See [LICENSE.txt](LICENSE.txt) for more information.

## 3rd party code

[gzcom-dll](https://github.com/nsgomez/gzcom-dll/tree/master) Located in the vendor folder, MIT License.    
[EABase](https://github.com/electronicarts/EABase) Located in the vendor folder, BSD 3-Clause License.    
[EASTL](https://github.com/electronicarts/EASTL) Located in the vendor folder, BSD 3-Clause License.    
[Windows Implementation Library](https://github.com/microsoft/wil) - MIT License.    

# Source Code

## Prerequisites

* Visual Studio 2022

## Building the plugin

* Open the solution in the `src` folder
* Update the post build events to copy the build output to you SimCity 4 application plugins folder.
* Build the solution

## Debugging the plugin

Visual Studio can be configured to launch SimCity 4 on the Debugging page of the project properties.
I configured the debugger to launch the game in full screen with the following command line:    
`-intro:off -CPUcount:1 -w -CustomResolution:enabled -r1920x1080x32`

You may need to adjust the window resolution for your primary screen.
