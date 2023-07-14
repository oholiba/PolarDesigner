//
//  EndlessSlider.h
//  PolarDesigner - All
//
//  Created by Jay Vaughan on 23.02.22.
//  Copyright © 2022 Austrian Audio. All rights reserved.
//
// This Component implements an 'endless slider', which can be useful for implementing
// 'trim' controls, i.e. for applying trim to a set of EQ's, volume sliders, etc.
//
// To adjust the rate of trim, use the EndlessSlider.step value in the slider inc/dec
// callbacks.

#ifndef EndlessSlider_h
#define EndlessSlider_h

#include "../resources/lookAndFeel/MainLookAndFeel.h"

class EndlessSlider : public Slider {
public:
    EndlessSlider () :
    Slider()
    {
        currentMoved = 0;
        lastMoved = 0;
        mouseWheelMovedCounter = 0;
    };
    
    // Trim step value - modify it freely as needed
    double step = 0.012725f;
    
    // set these callbacks where you use this class in order to get inc/dec messages
    std::function<void()> sliderIncremented;
    std::function<void()> sliderDecremented;
    
    // calculate whether to callback to an increment or decrement, and update UI
    void mouseDrag(const MouseEvent &e) override
    {
        if (e.mouseWasDraggedSinceMouseDown()) 
        {
            currentMoved = e.getDistanceFromDragStartY();
            if ((currentMoved > lastMoved))
            {
                sliderDecremented();
            }
            else if (currentMoved < lastMoved)
            {
                sliderIncremented();
            }
            lastMoved = currentMoved;
            repaint();
        }
    }

    void mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel) override
    {
        Rectangle<float> bounds = getLocalBounds().toFloat();
        float height = bounds.getHeight();

        mouseWheelMovedCounter++;
        currentMoved = wheel.deltaY * height * mouseWheelMovedCounter;

        if ((currentMoved > lastMoved))
        {
            sliderDecremented();
        }
        else if (currentMoved < lastMoved)
        {
            sliderIncremented();
        }
        lastMoved = currentMoved;

        repaint();
    }

    void paint (Graphics&g) override
    {
        g.fillAll(Colours::black);

        Rectangle<float> bounds = getLocalBounds().toFloat();
        float height = bounds.getHeight();
        int numElem = 34;
        float spaceBetween = height / static_cast<float>(numElem);
        float y = currentMoved;
        int mappedY = 0;
        int elemWidth = 0;
        int counter = 0;
        int r = sqrt((height * height) / 2); // circle radius

        ColourGradient cg = ColourGradient(mainLaF.trimSliderMainColor, 
                                           bounds.getWidth() / 2, 
                                           bounds.getHeight() / 2, 
                                           Colours::black, 
                                           bounds.getWidth() / 2, 0, true);
        g.setGradientFill(cg);
        g.fillRect(bounds.reduced(5, 5));

        for (int i = 0; i < numElem; i++)
        {
            if (i == 0)
            {
                y += spaceBetween/2; // place first element
            }
            else
            {
                y += spaceBetween;
            }
            // calculate y when mouse out of component
            if (y > height)
            {
                counter = std::abs(y / height);
                y -= height *counter;
            }
            else if (y < 0)
            {
                counter = std::abs(y / height) + 1;
                y += height * counter;
            }
            // calculate y when mousePos in component
            if (y < height /2)
            {
                mappedY = (-1) * (height / 2) + y;
            }
            else if (y == height / 2)
            {
                mappedY = 0;
            }
            else if (y > height / 2)
            {
                mappedY = y - height / 2;
            }
            // calculate width change with circle equation
            elemWidth = sqrt(r*r - (mappedY*mappedY)); 

            g.setColour(i == 17 ? Colours::grey : Colours::black);
            Rectangle<float> fillRect(bounds.getWidth()*0.22f, 
                                      y - (elemWidth / (numElem*2))/2, 
                                      bounds.getWidth()*0.55f, 
                                      elemWidth / (numElem * 2));
            g.fillRoundedRectangle(fillRect, 2.f);
        }
    }

    void mouseExit (const MouseEvent& e) override
    {
        repaint();
    }
    
    ~EndlessSlider () {}
    
    void resized() override
    {
        repaint();
    }

private:
    int lastMoved;
    int currentMoved;
    int mouseWheelMovedCounter;
    MainLookAndFeel mainLaF;
};

#endif /* EndlessSlider_h */
