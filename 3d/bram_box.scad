
height = 90; width = 90; depth = 28;
thickness = 2.7; cornerRadius = 6; tabLength = 10;

slotRepeatMin=1.2; slotLengthMin=26; slotLengthGap = 3.0; slotWidth = 0.2;

PI = 3.142*1.02; //Add a fudge factor for the bend radius
function hingeLength(angle, radius) = 2*PI*radius*(angle/360);

//Use true to generate 3D models of the box parts
//Use false to generate 2D models which can be exported
if(false)
{
  //Draws a folded version of the box
  translate([-width,depth+width,0])
    makeBox(false);

  //Draws a flat version of the box
  translate([width+height,depth+width,0])
    makeBox(true);
}
else
{
  // Projection allows it to draw a 2D version of the box
  // Which can be saved as SVG
  projection()
    translate([width+height,depth+width,0])
      makeBox(true);
}

// Generates the box in flat or folded parts
module makeBox(flat)
{
  if(flat)
  {
    union() 
    {
      translate([-(width+(4*thickness))/2 -thickness -1, 0, 0])
        boxSide2D(width+(4*thickness), depth, height, thickness, cornerRadius, false);

      translate([height/2 + thickness +1, 0, 0])
        boxSide2D(height, depth, width, thickness, cornerRadius, true);
    }
  }
  else
  {
    rotate([0,0,90])
    {
      translate([50,0,0])
        boxSide3D(height, width, depth, thickness, cornerRadius, true);

      translate([-50,0,0])
      rotate([90,0,180])
        boxSide3D(width+(4*thickness), height, depth, thickness,    cornerRadius, false);
    }
  }
}

module boxSide3D(height, width, depth, thickness, cornerRadius, tabsOut)
{
  faceWidth1 = depth-(2*cornerRadius);
  faceWidth2 = width-(2*cornerRadius);
  faceHeight = height;

  translate([(faceWidth1+thickness)/2 + cornerRadius,0,0])
  {
    translate([0,(faceWidth2+thickness)/2 + cornerRadius,0])
      livingHinge3D(90, cornerRadius, faceHeight, thickness);
      
      translate([0,-(faceWidth2+thickness)/2 + -cornerRadius,0])
      rotate([0,0,-90])
      livingHinge3D(90, cornerRadius, faceHeight, thickness);
      
    rotate([0,90,0])
      tabPanel(faceHeight, faceWidth2, thickness, tabLength, tabsOut);
  }
   
  rotate([-90,90,0])
  {
    translate([0,0,(faceWidth2+thickness)/2 + cornerRadius])
      boxEnd(faceHeight, faceWidth1, thickness, tabLength, cornerRadius, tabsOut);
        
    translate([0,0,-(faceWidth2+thickness)/2 - cornerRadius])
      boxEnd(faceHeight, faceWidth1, thickness, tabLength, cornerRadius, tabsOut);
  }
}

module boxSide2D(height, width, depth, thickness, cornerRadius, tabsOut)
{
  faceWidth1 = depth-(2*cornerRadius);
  faceWidth2 = width-(2*cornerRadius);
  hingeLength1 = hingeLength(90, cornerRadius);
  union()
  {
    tabPanel(height, faceWidth1, thickness, tabLength, tabsOut);

      translate([0,(faceWidth1 + hingeLength1)/2,0])
      {
        livingHinge2D(hingeLength1, height, thickness);
      
        translate([0,(hingeLength1 + faceWidth2)/2,0])
          boxEnd(height, faceWidth2, thickness, tabLength, cornerRadius, tabsOut);
      }

    mirror([0,1,0])
      translate([0,(faceWidth1 + hingeLength1)/2,0])
      {
        livingHinge2D(hingeLength1, height, thickness);
      
        translate([0,(hingeLength1 + faceWidth2)/2,0])
          boxEnd(height, faceWidth2, thickness, tabLength, cornerRadius, tabsOut);
      }
  } 
}

module boxEnd(height, width, thickness, tabLength, cornerRadius, tabsOut)
{
  tabPanel(height, width, thickness, tabLength, tabsOut);
 
  translate([0,width/2,0])
    tabbedEnd( height, thickness, cornerRadius, tabsOut);        
}


module tabPanel(panelHeight, panelWidth, panelThickness, tabLength, tabsOut=false)
{
  noTabsX = panelWidth /tabLength;
  noTabs = floor(noTabsX/2)+floor(noTabsX)%2;
  
  if(tabsOut)  
  union()
  {
    cube([panelHeight, panelWidth , panelThickness], true);

    union()
    {
      translate([(panelHeight+panelThickness)/2,0,0])
        rotate([0,0,90])
          makeTabs(noTabs, tabLength, panelThickness);

      translate([(-panelHeight-panelThickness)/2,0,0])
        rotate([0,0,90])
          makeTabs(noTabs, tabLength, panelThickness);
    }
  }
  
  else
  difference()
  {
    cube([panelHeight, panelWidth , panelThickness], true);

    union()
    {
      translate([(panelHeight-(3*panelThickness))/2,0,0])
        rotate([0,0,90])
          makeTabs(noTabs, tabLength, panelThickness);

      translate([(-panelHeight+(3*panelThickness))/2,0,0])
        rotate([0,0,90])
          makeTabs(noTabs, tabLength, panelThickness);
    }
  }
}

module livingHinge2D(panelLength, panelWidth, panelThickness) 
{
  widthDiv = floor(panelWidth/slotLengthMin);
  noSlots = floor(panelLength/slotRepeatMin)-1;
  slotRepeat = (panelWidth/widthDiv); 
  slotLength = panelLength / (noSlots + 1);

  difference() 
  {
    cube([panelWidth, panelLength, panelThickness], true);
    
    translate([-panelWidth/2,-panelLength/2,-panelThickness/2])
    for(y=[0:(widthDiv*2)-1]) 
    {           
      for (x =[1:noSlots])
      {
        if(x%2)
          translate([y*slotRepeat,0,0])
            translate([slotLengthGap*(y%2),(slotLength *x)-slotWidth/2,0])
              cube([slotRepeat-slotLengthGap, slotWidth, panelThickness]);
        else
          translate([y*slotRepeat,0,0])
            translate([slotLengthGap*(1-(y%2)),(slotLength *x)-slotWidth/2,0])
              cube([slotRepeat-slotLengthGap, slotWidth, panelThickness]);
      }
    }
  }
}

module livingHinge3D(angle, radius, panelWidth, panelThickness) 
{
  module pie(radius, angle, height, spin=0) 
  {
    // submodules
    module pieCube() 
    {
      translate([-radius - 1, 0, -1]) 
        cube([2*(radius + 1), radius, height + 2]);
    }
        
    ang = abs(angle % 360);
    negAng = angle < 0 ? angle : 0;
        
    rotate([0,0,negAng + spin]) 
    {
      if (angle == 0) 
        cylinder(r=radius, h=height, $fn=48);
      
      else if (abs(angle) > 0 && ang <= 180) 
      {
        difference() 
        {
          intersection() 
          {
            cylinder(r=radius, h=height, $fn=48);
            translate([0,0,0]) 
              pieCube();
          }
          
          rotate([0, 0, ang])
            pieCube();
        }
      } 
      else if (ang > 180) 
      {
        intersection() 
        {
          cylinder(r=radius, h=height, $fn=48);
          union() 
          {
            translate([0, 0, 0])
              pieCube();
            
            rotate([0, 0, ang - 180])
              pieCube();
          }
        }
      }
    }
  }
  
  translate([-(radius+(panelThickness/2)),-(radius+(panelThickness/2)),-panelWidth/2])
    rotate([0,0,0])
      difference() 
      {
        pie(radius+panelThickness, angle, panelWidth, spin = 0);
        pie(radius, angle, panelWidth, spin = 0);
      }
}

module tabbedEnd(panelWidth, panelThickness, radius, tabsOut) 
{
  if(tabsOut)
    roundedEnd(panelWidth-(2*radius), panelThickness, radius, tabsOut);
  else
    roundedEnd(panelWidth-(2*radius)-(4*panelThickness), panelThickness, radius+(2*panelThickness), tabsOut);
}

module roundedEnd(faceWidth, panelThickness, radius, tabsOut) 
{
  noTabsX = faceWidth /tabLength;
  noTabs = floor(noTabsX/2)+floor(noTabsX)%2;
 
  if(tabsOut)
  {
    union() 
    {
      translate([0,0,-panelThickness/2])
        roundedInsideEnd(faceWidth+(2*radius), panelThickness, radius);
        
      translate([0,radius+panelThickness/2,0])
        makeTabs(noTabs, tabLength, panelThickness);
    }
  }
  else
  {
    difference() 
    {
      translate([0,0,-panelThickness/2])
        roundedInsideEnd(faceWidth+(2*radius), panelThickness, radius);
    
      translate([0,radius-(1.5*panelThickness),0])
        makeTabs(noTabs, tabLength, panelThickness+.1);
    }
  }
}

module roundedInsideEnd(panelWidth, panelThickness, radius) 
{
  sub = 48;
  faceWidth = panelWidth - (2*radius);
  intersection() 
  {
    translate([-panelWidth/2,0,0])
      cube([panelWidth, radius, panelThickness]);
    
    union() 
    {
      translate([-faceWidth/2,0,0])
        cube([faceWidth, radius, panelThickness]);
      translate([-panelWidth/2 + radius,0,0])
        cylinder(r=radius, h=panelThickness, $fn=sub);
      translate([panelWidth/2 - radius,0,0])
        cylinder(r=radius, h=panelThickness, $fn=sub);
    }
  }
}

module makeTabs(noTabs, tabLength, panelThickness)
{
  union()
    for (i =[-noTabs+1:2:noTabs-1])
      translate([(i*tabLength),0,0])
        cube([tabLength, panelThickness, panelThickness], true);
}