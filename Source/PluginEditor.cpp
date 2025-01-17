/*
 ==============================================================================
 PluginEditor.cpp
 Author: Thomas Deppisch & Simon Beck
 
 Copyright (c) 2019 - Austrian Audio GmbH
 www.austrian.audio
 
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ==============================================================================
 */

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "../resources/customComponents/ImgPaths.h"

//==============================================================================
PolarDesignerAudioProcessorEditor::PolarDesignerAudioProcessorEditor (PolarDesignerAudioProcessor& p,
                                                                      AudioProcessorValueTreeState& vts)
: AudioProcessorEditor (&p), loadingFile(false), processor (p), valueTreeState(vts),
directivityEqualiser (p), alOverlayError(AlertOverlay::Type::errorMessage),
alOverlayDisturber(AlertOverlay::Type::disturberTracking),
alOverlaySignal(AlertOverlay::Type::signalTracking)
{
    //    openGLContext.attachTo (*getTopLevelComponent());
    
    nActiveBands = processor.getNBands();
    syncChannelIdx = processor.getSyncChannelIdx();

    setResizable(true, true);
    setSize(EDITOR_WIDTH, EDITOR_HEIGHT);
    setLookAndFeel(&globalLaF);

    addAndMakeVisible(&logoAA);
    addAndMakeVisible (&titleAA);
    addAndMakeVisible(&titlePD);
    titlePD.setTitle(String("PolarDesigner"));
    titlePD.setFont(globalLaF.aaRegular);
    addAndMakeVisible(&titleLine);

    addAndMakeVisible (&footer);
    
    addAndMakeVisible (&alOverlayError);
    alOverlayError.setVisible(false);
    alOverlayError.setColour(AlertWindow::backgroundColourId, globalLaF.AAGrey);
    alOverlayError.setColour(TextButton::buttonColourId, globalLaF.AARed);
    
    addAndMakeVisible (&alOverlayDisturber);
    alOverlayDisturber.setVisible(false);
    alOverlayDisturber.setColour(AlertWindow::backgroundColourId, globalLaF.AAGrey);
    alOverlayDisturber.setColour(TextButton::buttonColourId , globalLaF.AARed);
    alOverlayDisturber.setTitle("acquiring target!");
    alOverlayDisturber.setMessage("Make sure playback of an undesired target signal (spill) is active. Terminate to apply polar patterns with minimum spill energy. Also track a desired signal to be able to maximize the target-to-spill ratio.");
    
    addAndMakeVisible (&alOverlaySignal);
    alOverlaySignal.setVisible(false);
    alOverlaySignal.setColour(AlertWindow::backgroundColourId, globalLaF.AAGrey);
    alOverlaySignal.setColour(TextButton::buttonColourId , globalLaF.AARed);
    alOverlaySignal.setTitle("acquiring target!");
    alOverlaySignal.setMessage("Make sure playback of a desired target signal is active. Stop signal tracking to apply polar patterns with maximum signal energy. Also track an undesired spill target to be able to maximize the target-to-spill ratio.");
    
    // groups
    addAndMakeVisible (&grpEq);
    grpEq.setText ("equalization control");
    grpEq.setTextLabelPosition (Justification::centredLeft);
    
    addAndMakeVisible (&grpBands);
    grpBands.setText ("band control");
    grpBands.setTextLabelPosition (Justification::centredLeft);
    
    addAndMakeVisible (&grpPreset);
    grpPreset.setText ("preset control");
    grpPreset.setTextLabelPosition (Justification::centredLeft);
    
    addAndMakeVisible (&grpDstC);
    grpDstC.setText ("terminator control");
    grpDstC.setTextLabelPosition (Justification::centredLeft);
    
    addAndMakeVisible (&grpProxComp);
    grpProxComp.setText ("proximity control");
    grpProxComp.setTextLabelPosition (Justification::centredLeft);
    
    addAndMakeVisible (&grpSync);
    grpSync.setText ("sync-channel");
    grpSync.setTextLabelPosition (Justification::centredLeft);
    
    eqColours[0] = Colour(0xFDBA4949);
    eqColours[1] = Colour(0xFDBA6F49);
    eqColours[2] = Colour(0xFDBAAF49);
    eqColours[3] = Colour(0xFD8CBA49);
    eqColours[4] = Colour(0xFD49BA64);
    
    // directivity eq
    addAndMakeVisible (&directivityEqualiser);
    
    for (int i = 0; i < maxNumberBands; ++i)
    {
        // SOLO button
        msbSolo[i].setType (MuteSoloButton::Type::solo);
        addAndMakeVisible (&msbSolo[i]);
        msbSoloAtt[i] = std::unique_ptr<ButtonAttachment>(new ButtonAttachment (valueTreeState, "solo" + String(i+1), msbSolo[i]));
        msbSolo[i].addListener (this);
        msbSolo[i].setAlwaysOnTop (true);
        
        // MUTE button
        msbMute[i].setType (MuteSoloButton::Type::mute);
        addAndMakeVisible (&msbMute[i]);
        msbMuteAtt[i] = std::unique_ptr<ButtonAttachment>(new ButtonAttachment (valueTreeState, "mute" + String(i+1), msbMute[i]));
        msbMute[i].addListener (this);
        msbMute[i].setAlwaysOnTop (true);
        
        // Direction slider
        addAndMakeVisible (&slDir[i]);
        slDirAtt[i] = std::unique_ptr<SliderAttachment>(new SliderAttachment (valueTreeState, "alpha" + String(i+1), slDir[i]));
        slDir[i].setColour (Slider::thumbColourId, eqColours[i]); // colour of knob
        slDir[i].addListener (this);
        slDir[i].setTooltipEditable (true);
        
        // Band Gain slider
        addAndMakeVisible (&slBandGain[i]);
        slBandGainAtt[i] = std::unique_ptr<ReverseSlider::SliderAttachment>(new ReverseSlider::SliderAttachment (valueTreeState, "gain" + String(i+1), slBandGain[i]));
        slBandGain[i].setSliderStyle (Slider::LinearHorizontal);
        slBandGain[i].setColour (Slider::rotarySliderOutlineColourId, eqColours[i]);
        slBandGain[i].setColour (Slider::thumbColourId, eqColours[i]);
        slBandGain[i].setTextBoxStyle (Slider::TextBoxAbove, false, 50, 15);
        slBandGain[i].addListener (this);
        
        // First-Order directivity visualizer (The "O"verhead view)
        addAndMakeVisible (&polarPatternVisualizers[i]);
        polarPatternVisualizers[i].setActive(true);
        polarPatternVisualizers[i].setDirWeight (slDir[i].getValue());
        polarPatternVisualizers[i].setMuteSoloButtons (&msbSolo[i], &msbMute[i]);
        polarPatternVisualizers[i].setColour (eqColours[i]);
        
        // main directivity Equaliser section
        directivityEqualiser.addSliders (eqColours[i], &slDir[i], (i > 0) ? &slCrossoverPosition[i - 1] : nullptr, (i < maxNumberBands - 1) ? &slCrossoverPosition[i] : nullptr, &msbSolo[i], &msbMute[i], &slBandGain[i], &polarPatternVisualizers[i]);
        
        if (i == maxNumberBands - 1)
            break; // there is one slCrossoverPosition less than bands
        
        addAndMakeVisible (&slCrossoverPosition[i]);
        slCrossoverAtt[i] = std::unique_ptr<ReverseSlider::SliderAttachment>(new ReverseSlider::SliderAttachment (valueTreeState, "xOverF" + String(i+1), slCrossoverPosition[i]));
        slCrossoverPosition[i].setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
        slCrossoverPosition[i].addListener(this);
        slCrossoverPosition[i].setVisible(false);
    }
    
    directivityEqualiser.initValueBox();
    
    addAndMakeVisible (&tbLoadFile);
    tbLoadFile.setButtonText ("load preset");
    tbLoadFile.addListener (this);
    
    addAndMakeVisible (&tbSaveFile);
    tbSaveFile.setButtonText ("save preset");
    tbSaveFile.addListener (this);
    
    addAndMakeVisible (&tbRecordDisturber);
    tbRecordDisturber.setButtonText ("terminate spill");
    tbRecordDisturber.addListener (this);
    
    addAndMakeVisible (&tbRecordSignal);
    tbRecordSignal.setButtonText ("maximize target");
    tbRecordSignal.addListener (this);
    
    addAndMakeVisible (&tbAllowBackwardsPattern);
    tbAllowBackwardsPatternAtt = std::unique_ptr<ButtonAttachment>(new ButtonAttachment (valueTreeState, "allowBackwardsPattern", tbAllowBackwardsPattern));
    tbAllowBackwardsPattern.setButtonText ("allow reverse patterns");
    tbAllowBackwardsPattern.addListener (this);
    
    addAndMakeVisible (&tbEq[0]);
    tbEq[0].addListener (this);
    tbEq[0].setButtonText ("off");
    tbEq[0].setRadioGroupId(1);
    
    addAndMakeVisible (&tbEq[1]);
    tbEq[1].addListener (this);
    tbEq[1].setButtonText ("free field");
    tbEq[1].setRadioGroupId(1);
    
    addAndMakeVisible (&tbEq[2]);
    tbEq[2].addListener (this);
    tbEq[2].setButtonText ("diffuse field");
    tbEq[2].setRadioGroupId(1);
    
    addAndMakeVisible (&tbAbButton[0]);
    tbAbButton[0].addListener (this);
    tbAbButton[0].setButtonText("A");
    tbAbButton[0].setToggleState(processor.abLayerState, NotificationType::dontSendNotification);
    tbAbButton[0].setClickingTogglesState(true);
    tbAbButton[0].setAlpha(getABButtonAlphaFromLayerState(processor.abLayerState));
    tbAbButton[0].setRadioGroupId(2);
    
    addAndMakeVisible (&tbAbButton[1]);
    tbAbButton[1].addListener(this);
    tbAbButton[1].setButtonText("B");
    tbAbButton[1].setToggleState(!processor.abLayerState, NotificationType::dontSendNotification);
    tbAbButton[1].setClickingTogglesState(true);
    tbAbButton[1].setAlpha(getABButtonAlphaFromLayerState(!processor.abLayerState));
    tbAbButton[1].setRadioGroupId(2);
    
    
#if 0
    {
        //    cbSetNrBands.setLookAndFeel(&comboBoxLaF);
        
        addAndMakeVisible (&cbSetNrBands);
        cbSetNrBandsAtt = std::unique_ptr<ComboBoxAttachment>(new ComboBoxAttachment (valueTreeState, "nrBands", cbSetNrBands));
        cbSetNrBands.setEditableText (false);
        cbSetNrBands.addItemList (juce::StringArray ({"one band","two bands",
            "three bands","four bands",
            "five bands"}), 1);
        cbSetNrBands.setJustificationType (Justification::centred);
        cbSetNrBands.setSelectedId (nActiveBands);
        cbSetNrBands.addListener (this);
    }
#endif
    
    // TODO: Replace cbSetNrBands with this:
    for (int i = 0; i < maxNumberBands; ++i)
    {
        //        tbSetNrBands[i].setTitle(String (i + 1));
        addAndMakeVisible(tbSetNrBands[i]);
        //        tbSetNrBandsAtt[i] = std::unique_ptr<ButtonAttachment>(new ButtonAttachment (valueTreeState, "nrBands", tbSetNrBands[i]));
        
        tbSetNrBands[i].setClickingTogglesState (true);
        tbSetNrBands[i].setRadioGroupId (34567);
        
        //        tbSetNrBands[i].setColour (TextButton::textColourOffId,  Colours::black);
        tbSetNrBands[i].setColour (TextButton::textColourOnId,   Colours::powderblue);
        //        tbSetNrBands[i].setColour (TextButton::buttonColourId,   Colours::white);
        tbSetNrBands[i].setColour (TextButton::buttonOnColourId, Colours::blueviolet.brighter());
        
        tbSetNrBands[i].setConnectedEdges (((i != 0) ? Button::ConnectedOnLeft : 0)
                                           | ((i != 3) ? Button::ConnectedOnRight : 0));
        
        
        tbSetNrBands[i].addListener(this);
        
        if (i == (nActiveBands - 1)) {
            tbSetNrBands[i].setToggleState (true, NotificationType::dontSendNotification);
            
        }
    }
    
#if 0
    addAndMakeVisible (&cbSyncChannel);
    cbSyncChannelAtt = std::unique_ptr<ComboBoxAttachment>(new ComboBoxAttachment (valueTreeState, "syncChannel", cbSyncChannel));
    cbSyncChannel.setEditableText (false);
    cbSyncChannel.addItemList (juce::StringArray ({"none","one","two","three","four"}), 1);
    cbSyncChannel.setJustificationType (Justification::centred);
    cbSyncChannel.setSelectedId (syncChannelIdx);
    cbSyncChannel.addListener (this);
#endif
    
    // TODO: Replace cbSyncChannel with this:
    for (int i = 0; i < 5; ++i)
    {
        
        //        if (i == 0) {
        //            tbSyncChannel[i].setTitle(String ("X"));
        //        }
        //        else {
        //            tbSyncChannel[i].setTitle(String (i));
        //        }
        
        addAndMakeVisible(tbSyncChannel[i]);
        
        tbSyncChannel[i].setClickingTogglesState (true);
        tbSyncChannel[i].setRadioGroupId (76543);
        
        tbSyncChannel[i].setColour (TextButton::textColourOnId,   Colours::powderblue);
        tbSyncChannel[i].setColour (TextButton::buttonOnColourId, Colours::blueviolet.brighter());
        
        tbSyncChannel[i].setConnectedEdges (((i != 0) ? Button::ConnectedOnLeft : 0)
                                            | ((i != 3) ? Button::ConnectedOnRight : 0));
        
        
        tbSyncChannel[i].addListener(this);
        
        if (i == syncChannelIdx - 1) {
            tbSyncChannel[i].setToggleState (true, NotificationType::dontSendNotification);
            
        }
    }
    
    
    
    
    
    addAndMakeVisible (&slProximity);
    slProximityAtt = std::unique_ptr<ReverseSlider::SliderAttachment>(new ReverseSlider::SliderAttachment (valueTreeState, "proximity", slProximity));
    slProximity.setSliderStyle (Slider::LinearHorizontal);
    slProximity.setColour (Slider::thumbColourId, globalLaF.AARed);
    slProximity.setColour (Slider::rotarySliderOutlineColourId, globalLaF.AARed);
    slProximity.setTextBoxStyle (Slider::TextBoxRight, false, 45, 15);
    slProximity.addListener (this);
    
    addAndMakeVisible (&tbZeroDelay);
    tbZeroDelayAtt = std::unique_ptr<ButtonAttachment>(new ButtonAttachment (valueTreeState, "zeroDelayMode", tbZeroDelay));
    tbZeroDelay.addListener (this);
    tbZeroDelay.setButtonText ("zero latency");
    tbZeroDelay.setToggleState(processor.zeroDelayModeActive(), NotificationType::dontSendNotification);
    
    directivityEqualiser.setSoloActive (getSoloActive());
    for (auto& vis : polarPatternVisualizers)
    {
        vis.setSoloActive (getSoloActive());
    }
    
    
    // set overlay callbacks
    alOverlayError.setOnOkayCallback ([this]() { onAlOverlayErrorOkay(); });
    
    alOverlayDisturber.setOnOkayCallback ([this]() { onAlOverlayApplyPattern(); });
    alOverlayDisturber.setOnCancelCallback ([this]() { onAlOverlayCancelRecord(); });
    alOverlayDisturber.setOnRatioCallback ([this]() { onAlOverlayMaxSigToDist(); });
    
    alOverlaySignal.setOnOkayCallback ([this]() { onAlOverlayApplyPattern(); });
    alOverlaySignal.setOnCancelCallback ([this]() { onAlOverlayCancelRecord(); });
    alOverlaySignal.setOnRatioCallback ([this]() { onAlOverlayMaxSigToDist(); });
    
    nActiveBandsChanged();
    
    trimSlider.sliderIncremented = [this] { incrementTrim(nActiveBands); };
    trimSlider.sliderDecremented = [this] { decrementTrim(nActiveBands); };
    
    addAndMakeVisible(&trimSlider);
    
    nActiveBandsChanged();
    zeroDelayModeChange();
    
    trimSlider.sliderIncremented = [this] { incrementTrim(this->nActiveBands); };
    trimSlider.sliderDecremented = [this] { decrementTrim(this->nActiveBands); };

    startTimer (30);
    
    setEqMode();
    
    
}

// Handle the trimSlider increment/decrement calls
void PolarDesignerAudioProcessorEditor::incrementTrim(int nBands) {
    for (int i = 0; i < nBands; i++)
    {
        slDir[i].setValue(slDir[i].getValue() + trimSlider.step);
    }
}

void PolarDesignerAudioProcessorEditor::decrementTrim(int nBands) {
    for (int i = 0; i < nBands; i++)
    {
        slDir[i].setValue(slDir[i].getValue() - trimSlider.step);
    }
}

PolarDesignerAudioProcessorEditor::~PolarDesignerAudioProcessorEditor()
{
    if (alOverlayDisturber.isVisible())
        onAlOverlayCancelRecord();
    
    if (alOverlaySignal.isVisible())
        onAlOverlayCancelRecord();
    
    setLookAndFeel (nullptr);
    
}

//==============================================================================
void PolarDesignerAudioProcessorEditor::paint (Graphics& g)
{
    g.fillAll (globalLaF.ClBackground);
    
#ifdef AA_DO_DEBUG_PATH
    g.strokePath (debugPath, PathStrokeType (15.0f));
#endif
    
}

void PolarDesignerAudioProcessorEditor::resized()
{

    Rectangle<int> area (getLocalBounds());

    juce::FlexBox fb;
    fb.flexDirection = FlexBox::Direction::column;
    fb.justifyContent = juce::FlexBox::JustifyContent::center;
    fb.alignContent = juce::FlexBox::AlignContent::center;

    juce::FlexBox topComponent;
    topComponent.flexDirection = FlexBox::Direction::row;
    topComponent.justifyContent = juce::FlexBox::JustifyContent::center;
    topComponent.alignContent = juce::FlexBox::AlignContent::center;

    const float marginFlex = 0.01f;
    const float topComponentTitleFlex = 0.4f;
    const float topComponentButtonsFlex = 0.05f;
    const float topComponentSpacingFlex = topComponentButtonsFlex/2;
    const float topComponentButtonsMargin = 5;

    topComponent.items.add(juce::FlexItem().withFlex(marginFlex));
    topComponent.items.add(juce::FlexItem(logoAA).withFlex(topComponentButtonsFlex));
    topComponent.items.add(juce::FlexItem().withFlex(topComponentSpacingFlex));
    topComponent.items.add(juce::FlexItem(titleAA).withFlex(topComponentTitleFlex));
    topComponent.items.add(juce::FlexItem().withFlex(topComponentSpacingFlex));
    topComponent.items.add(juce::FlexItem(titlePD).withFlex(topComponentTitleFlex));
    topComponent.items.add(juce::FlexItem(tbAbButton[0]).withFlex(topComponentButtonsFlex).withMargin(topComponentButtonsMargin));
    topComponent.items.add(juce::FlexItem().withFlex(topComponentSpacingFlex/2));
    topComponent.items.add(juce::FlexItem(tbAbButton[1]).withFlex(topComponentButtonsFlex).withMargin(topComponentButtonsMargin));
    topComponent.items.add(juce::FlexItem().withFlex(topComponentSpacingFlex));
    topComponent.items.add(juce::FlexItem(tbZeroDelay).withFlex(topComponentButtonsFlex*3).withMargin(5));
    topComponent.items.add(juce::FlexItem().withFlex(marginFlex));

    juce::FlexBox topComponentLine;
    topComponentLine.flexDirection = FlexBox::Direction::row;
    topComponentLine.justifyContent = juce::FlexBox::JustifyContent::center;
    topComponentLine.alignContent = juce::FlexBox::AlignContent::center;
    topComponentLine.items.add(juce::FlexItem().withFlex(marginFlex));
    topComponentLine.items.add(juce::FlexItem(titleLine).withFlex(1.f - 2 * marginFlex));
    topComponentLine.items.add(juce::FlexItem().withFlex(marginFlex));

    const float sideComponentItemFlex = 0.05f;

    juce::FlexBox sideComponent;
    sideComponent.flexDirection = FlexBox::Direction::column;
    sideComponent.justifyContent = juce::FlexBox::JustifyContent::center;
    sideComponent.alignContent = juce::FlexBox::AlignContent::center;
    sideComponent.items.add(juce::FlexItem().withFlex(marginFlex));
    sideComponent.items.add(juce::FlexItem(grpBands).withFlex(sideComponentItemFlex));
    sideComponent.items.add(juce::FlexItem(cbSetNrBands).withFlex(sideComponentItemFlex));
    sideComponent.items.add(juce::FlexItem().withFlex(marginFlex));
    sideComponent.items.add(juce::FlexItem(grpPreset).withFlex(sideComponentItemFlex));
    sideComponent.items.add(juce::FlexItem(tbLoadFile).withFlex(sideComponentItemFlex));
    sideComponent.items.add(juce::FlexItem(tbSaveFile).withFlex(sideComponentItemFlex));
    sideComponent.items.add(juce::FlexItem().withFlex(marginFlex));
    sideComponent.items.add(juce::FlexItem(grpEq).withFlex(sideComponentItemFlex));
    sideComponent.items.add(juce::FlexItem(tbEq[0]).withFlex(sideComponentItemFlex));
    sideComponent.items.add(juce::FlexItem(tbEq[1]).withFlex(sideComponentItemFlex));
    sideComponent.items.add(juce::FlexItem(tbEq[2]).withFlex(sideComponentItemFlex));
    sideComponent.items.add(juce::FlexItem().withFlex(marginFlex));
    sideComponent.items.add(juce::FlexItem(grpProxComp).withFlex(sideComponentItemFlex));
    sideComponent.items.add(juce::FlexItem(slProximity).withFlex(sideComponentItemFlex));
    sideComponent.items.add(juce::FlexItem().withFlex(marginFlex));
    sideComponent.items.add(juce::FlexItem(grpDstC).withFlex(sideComponentItemFlex));
    sideComponent.items.add(juce::FlexItem(tbAllowBackwardsPattern).withFlex(sideComponentItemFlex));
    sideComponent.items.add(juce::FlexItem(tbRecordDisturber).withFlex(sideComponentItemFlex));
    sideComponent.items.add(juce::FlexItem(tbRecordSignal).withFlex(sideComponentItemFlex));
    sideComponent.items.add(juce::FlexItem().withFlex(marginFlex));
    sideComponent.items.add(juce::FlexItem(grpSync).withFlex(sideComponentItemFlex));
    sideComponent.items.add(juce::FlexItem(cbSyncChannel).withFlex(sideComponentItemFlex));
    sideComponent.items.add(juce::FlexItem().withFlex(marginFlex));

    // Margins are fixed value because DirectivityEQ component has fixed margins
    const float polarVisualizersComponentLeftMargin = 33;
    const float polarVisualizersComponentRightMargin = 10;

    juce::FlexBox polarVisualizersComponent;
    polarVisualizersComponent.flexDirection = FlexBox::Direction::row;
    polarVisualizersComponent.justifyContent = juce::FlexBox::JustifyContent::center;
    polarVisualizersComponent.alignContent = juce::FlexBox::AlignContent::center;
    polarVisualizersComponent.items.add(juce::FlexItem().withWidth(polarVisualizersComponentLeftMargin));

    juce::FlexBox muteSoloModule;
    muteSoloModule.flexDirection = FlexBox::Direction::row;
    muteSoloModule.justifyContent = juce::FlexBox::JustifyContent::center;
    muteSoloModule.alignContent = juce::FlexBox::AlignContent::center;
    muteSoloModule.items.add(juce::FlexItem().withWidth(polarVisualizersComponentLeftMargin));

    const float muteSoloComponentButtonsFlex = 0.14f;

    juce::FlexBox muteSoloComponent[5];
    for (int i = 0; i < 5; i++)
    {
        muteSoloComponent[i].flexDirection = FlexBox::Direction::row;
        muteSoloComponent[i].justifyContent = juce::FlexBox::JustifyContent::center;
        muteSoloComponent[i].alignContent = juce::FlexBox::AlignContent::center;
        muteSoloComponent[i].items.add(juce::FlexItem().withFlex(marginFlex));
        muteSoloComponent[i].items.add(juce::FlexItem(msbMute[i]).withFlex(muteSoloComponentButtonsFlex));
        muteSoloComponent[i].items.add(juce::FlexItem().withFlex(1.f - 2 * marginFlex - 2 * muteSoloComponentButtonsFlex));
        muteSoloComponent[i].items.add(juce::FlexItem(msbSolo[i]).withFlex(muteSoloComponentButtonsFlex));
        muteSoloComponent[i].items.add(juce::FlexItem().withFlex(marginFlex));
    }

    juce::FlexBox dirSlidersComponent;
    dirSlidersComponent.flexDirection = FlexBox::Direction::row;
    dirSlidersComponent.justifyContent = juce::FlexBox::JustifyContent::center;
    dirSlidersComponent.alignContent = juce::FlexBox::AlignContent::center;
    dirSlidersComponent.items.add(juce::FlexItem().withWidth(polarVisualizersComponentLeftMargin));

    juce::FlexBox gainBandSlidersComponent;
    gainBandSlidersComponent.flexDirection = FlexBox::Direction::row;
    gainBandSlidersComponent.justifyContent = juce::FlexBox::JustifyContent::center;
    gainBandSlidersComponent.alignContent = juce::FlexBox::AlignContent::center;
    gainBandSlidersComponent.items.add(juce::FlexItem().withWidth(polarVisualizersComponentLeftMargin));

    //Dynamic layout for polarVisualizers and dirSlider components
    //offsetDirEQ and offsetPolVis are fixed values because DirectivityEQ component has fixed margins
    const float offsetDirEQ = 42;
    const float offsetPolVis = 29;

    const float dirEqSize = directivityEqualiser.getWidth() - offsetDirEQ;
    auto bandLimitWidth = getBandLimitWidthVector(dirEqSize, offsetPolVis);

    //pVisflex - value used for components spacing across given area i.e 0.65 (maximum 1.0 means full space)
    float pVisflex = 0;

    if (nActiveBands < 2)
    {
        if (polarPatternVisualizers[0].isPvisActive())
        {
            pVisflex = bandLimitWidth[0] / dirEqSize;
            polarVisualizersComponent.items.add(juce::FlexItem(polarPatternVisualizers[0]).withFlex(pVisflex));
            dirSlidersComponent.items.add(juce::FlexItem(slDir[0]).withFlex(pVisflex));
            muteSoloModule.items.add(juce::FlexItem(muteSoloComponent[0]).withFlex(pVisflex));
            gainBandSlidersComponent.items.add(juce::FlexItem(slBandGain[0]).withFlex(pVisflex));
        }
    }
    else
    {
        for (int i = 0; i < nActiveBands; i++)
        {
            if (polarPatternVisualizers[i].isPvisActive())
            {
                //TODO: modify the function so that there is no danger of going outside the array --> i+1
                pVisflex = bandLimitWidth[i+1] / dirEqSize;
                polarVisualizersComponent.items.add(juce::FlexItem(polarPatternVisualizers[i]).withFlex(pVisflex));
                dirSlidersComponent.items.add(juce::FlexItem(slDir[i]).withFlex(pVisflex));
                muteSoloModule.items.add(juce::FlexItem(muteSoloComponent[i]).withFlex(pVisflex));
                gainBandSlidersComponent.items.add(juce::FlexItem(slBandGain[i]).withFlex(pVisflex));
            }
        }
    }

    polarVisualizersComponent.items.add(juce::FlexItem().withWidth(polarVisualizersComponentRightMargin));
    dirSlidersComponent.items.add(juce::FlexItem().withWidth(polarVisualizersComponentRightMargin));
    muteSoloModule.items.add(juce::FlexItem().withWidth(polarVisualizersComponentRightMargin));
    gainBandSlidersComponent.items.add(juce::FlexItem().withWidth(polarVisualizersComponentRightMargin));

    const float middleComponentFlex = 0.05f;

    juce::FlexBox middleComponent;
    middleComponent.flexDirection = FlexBox::Direction::column;
    middleComponent.justifyContent = juce::FlexBox::JustifyContent::center;
    middleComponent.alignContent = juce::FlexBox::AlignContent::center;
    middleComponent.items.add(juce::FlexItem().withFlex(marginFlex));
    middleComponent.items.add(juce::FlexItem(polarVisualizersComponent).withFlex(middleComponentFlex*4));
    middleComponent.items.add(juce::FlexItem().withFlex(marginFlex));
    middleComponent.items.add(juce::FlexItem(directivityEqualiser).withFlex(middleComponentFlex*10));
    middleComponent.items.add(juce::FlexItem().withFlex(marginFlex));
    middleComponent.items.add(juce::FlexItem(dirSlidersComponent).withFlex(middleComponentFlex));
    middleComponent.items.add(juce::FlexItem(muteSoloModule).withFlex(middleComponentFlex));
    middleComponent.items.add(juce::FlexItem(gainBandSlidersComponent).withFlex(middleComponentFlex));
    middleComponent.items.add(juce::FlexItem().withFlex(marginFlex));

    const float trimSliderComponentFlex = 0.5f;
    const float trimSliderComponentMarginOffset = 0.03f;

    juce::FlexBox trimSliderComponent;
    trimSliderComponent.flexDirection = FlexBox::Direction::column;
    trimSliderComponent.justifyContent = juce::FlexBox::JustifyContent::center;
    trimSliderComponent.alignContent = juce::FlexBox::AlignContent::center;
    trimSliderComponent.items.add(juce::FlexItem().withFlex(trimSliderComponentFlex/2 + trimSliderComponentMarginOffset));
    trimSliderComponent.items.add(juce::FlexItem(trimSlider).withFlex(trimSliderComponentFlex));
    trimSliderComponent.items.add(juce::FlexItem().withFlex(trimSliderComponentFlex/2 - trimSliderComponentMarginOffset));
    
    juce::FlexBox mainComponent;
    mainComponent.flexDirection = FlexBox::Direction::row;
    mainComponent.justifyContent = juce::FlexBox::JustifyContent::center;
    mainComponent.alignContent = juce::FlexBox::AlignContent::center;
    mainComponent.items.add(juce::FlexItem().withFlex(marginFlex));
    mainComponent.items.add(juce::FlexItem(sideComponent).withFlex(marginFlex*15));
    mainComponent.items.add(juce::FlexItem().withFlex(marginFlex));
    mainComponent.items.add(juce::FlexItem(middleComponent).withFlex(marginFlex*75));
    mainComponent.items.add(juce::FlexItem().withFlex(marginFlex));
    mainComponent.items.add(juce::FlexItem(trimSliderComponent).withFlex(marginFlex*2));
    mainComponent.items.add(juce::FlexItem().withFlex(marginFlex));

    fb.items.add(juce::FlexItem().withFlex(marginFlex));
    fb.items.add(juce::FlexItem(topComponent).withFlex(marginFlex*10));
    fb.items.add(juce::FlexItem().withFlex(marginFlex/2));
    fb.items.add(juce::FlexItem(topComponentLine).withFlex(marginFlex/5));
    fb.items.add(juce::FlexItem().withFlex(marginFlex));
    fb.items.add(juce::FlexItem(mainComponent).withFlex(marginFlex*75));
    fb.items.add(juce::FlexItem(footer).withFlex(marginFlex*5));

    fb.performLayout(area);

    /*
    alOverlayError.setBounds (directivityEqualiser.getX() + 120, directivityEqualiser.getY() + 50, directivityEqualiser.getWidth() - 240, directivityEqualiser.getHeight() - 100);
    alOverlayDisturber.setBounds (directivityEqualiser.getX() + 120, directivityEqualiser.getY() + 50, directivityEqualiser.getWidth() - 240, directivityEqualiser.getHeight() - 100);
    alOverlaySignal.setBounds (directivityEqualiser.getX() + 120, directivityEqualiser.getY() + 50, directivityEqualiser.getWidth() - 240, directivityEqualiser.getHeight() - 100);
    */
}

void PolarDesignerAudioProcessorEditor::buttonStateChanged(Button* button)
{
    //    std::cout << "button:::" << button;
    
}

void PolarDesignerAudioProcessorEditor::buttonClicked (Button* button)
{
    if ((button == &tbSetNrBands[0]) && (button->getToggleState() > 0.5f))
    {
        valueTreeState.getParameter("nrBands")->setValueNotifyingHost(valueTreeState.getParameter("nrBands")->convertTo0to1((0)));
    }
    if ((button == &tbSetNrBands[1]) && (button->getToggleState() > 0.5f))
    {
        valueTreeState.getParameter("nrBands")->setValueNotifyingHost(valueTreeState.getParameter("nrBands")->convertTo0to1((1)));
    }
    if ((button == &tbSetNrBands[2]) && (button->getToggleState() > 0.5f))
    {
        valueTreeState.getParameter("nrBands")->setValueNotifyingHost(valueTreeState.getParameter("nrBands")->convertTo0to1((2)));
    }
    if ((button == &tbSetNrBands[3]) && (button->getToggleState() > 0.5f))
    {
        valueTreeState.getParameter("nrBands")->setValueNotifyingHost(valueTreeState.getParameter("nrBands")->convertTo0to1((3)));
    }
    if ((button == &tbSetNrBands[4]) && (button->getToggleState() > 0.5f))
    {
        valueTreeState.getParameter("nrBands")->setValueNotifyingHost(valueTreeState.getParameter("nrBands")->convertTo0to1((4)));
    }
    
    if ((button == &tbSyncChannel[0]) && (button->getToggleState() > 0.5f))
    {
        valueTreeState.getParameter("syncChannel")->setValueNotifyingHost(valueTreeState.getParameter("syncChannel")->convertTo0to1((1)));
    }
    if ((button == &tbSyncChannel[1]) && (button->getToggleState() > 0.5f))
    {
        valueTreeState.getParameter("syncChannel")->setValueNotifyingHost(valueTreeState.getParameter("syncChannel")->convertTo0to1((2)));
    }
    if ((button == &tbSyncChannel[2]) && (button->getToggleState() > 0.5f))
    {
        valueTreeState.getParameter("syncChannel")->setValueNotifyingHost(valueTreeState.getParameter("syncChannel")->convertTo0to1((3)));
    }
    if ((button == &tbSyncChannel[3]) && (button->getToggleState() > 0.5f))
    {
        valueTreeState.getParameter("syncChannel")->setValueNotifyingHost(valueTreeState.getParameter("syncChannel")->convertTo0to1((4)));
    }
    if ((button == &tbSyncChannel[4]) && (button->getToggleState() > 0.5f))
    {
        valueTreeState.getParameter("syncChannel")->setValueNotifyingHost(valueTreeState.getParameter("syncChannel")->convertTo0to1((5)));
    }

    
    if (button == &tbLoadFile)
    {
        loadFile();
    }
    else if (button == &tbSaveFile)
    {
        saveFile();
    }
    else if (button == &tbEq[0])
    {
        processor.setEqState(0);
    }
    else if (button == &tbEq[1])
    {
        processor.setEqState(1);
    }
    else if (button == &tbEq[2])
    {
        processor.setEqState(2);
    }
    else if (button == &tbRecordDisturber)
    {
        processor.startTracking(true);
        alOverlayDisturber.enableRatioButton(processor.getSignalRecorded());
        alOverlayDisturber.setVisible(true);
        disableMainArea();
        setSideAreaEnabled(false);
    }
    else if (button == &tbRecordSignal)
    {
        processor.startTracking(false);
        alOverlaySignal.enableRatioButton(processor.getDisturberRecorded());
        alOverlaySignal.setVisible(true);
        disableMainArea();
        setSideAreaEnabled(false);
    }
    else if (button == &tbAllowBackwardsPattern)
    {
        return;
    }
    else if (button == &tbZeroDelay)
    {
        bool isToggled = button->getToggleState();
        button->setToggleState(!isToggled, NotificationType::dontSendNotification);
    }
    else if (button == &tbAbButton[0])
    {
        bool isToggled = button->getToggleState();
        if (isToggled < 0.5f)
        {
            processor.setAbLayer(0);
            button->setAlpha(getABButtonAlphaFromLayerState(isToggled));
            tbAbButton[1].setAlpha(getABButtonAlphaFromLayerState(!isToggled));
        }
    }
    else if (button == &tbAbButton[1])
    {
        bool isToggled = button->getToggleState();
        if (isToggled < 0.5f)
        {
            processor.setAbLayer(1);
            button->setAlpha(getABButtonAlphaFromLayerState(isToggled));
            tbAbButton[0].setAlpha(getABButtonAlphaFromLayerState(!isToggled));
        }
    }
    else // muteSoloButton!
    {
        directivityEqualiser.setSoloActive(getSoloActive());
        directivityEqualiser.repaint();
        for (auto& vis : polarPatternVisualizers)
        {
            vis.setSoloActive(getSoloActive());
            vis.repaint();
        }
    }
}

float PolarDesignerAudioProcessorEditor::getABButtonAlphaFromLayerState(int layerState)
{
    return layerState * 0.7f + 0.3f;
}

std::vector<float> PolarDesignerAudioProcessorEditor::getBandLimitWidthVector(float dirEqSize, float offsetPolVis)
{
    //First calculate bandLimit vector
    std::vector<float> bandLimit;
    bandLimit.push_back(0);
    for (int i = 0; i < nActiveBands - 1; i++)
    {
        bandLimit.push_back(directivityEqualiser.getBandlimitPathComponent(i).getX() - offsetPolVis);
    }
    bandLimit.push_back(dirEqSize);
    //Next calculate width of each band
    std::vector<float>::iterator it;
    int i = 1;
    std::vector<float> bandLimitWidth;
    bandLimitWidth.push_back(dirEqSize);
    for (it = bandLimit.begin() + 1; it != bandLimit.end(); it++, i++) {
        bandLimitWidth.push_back(bandLimit[i] - bandLimit[i - 1]);
    }

    return bandLimitWidth;
}

bool PolarDesignerAudioProcessorEditor::getSoloActive()
{
    bool active = false;
    for (auto& but : msbSolo)
    {
        if (but.getToggleState())
        {
            active = true;
            break;
        }
    }
    return active;
}

void PolarDesignerAudioProcessorEditor::comboBoxChanged (ComboBox* cb)
{
    if (cb == &cbSetNrBands)
    {
        nActiveBands = cb->getSelectedId();
        for (int i = 0; i < 5; i++)
        {
            if (i < nActiveBands)
            {
                polarPatternVisualizers[i].setActive(true);
                slDir[i].setVisible(true);
                msbMute[i].setVisible(true);
                msbSolo[i].setVisible(true);
                slBandGain[i].setVisible(true);
            }
            else
            {
                polarPatternVisualizers[i].setActive(false);
                slDir[i].setVisible(false);
                msbMute[i].setVisible(false);
                msbSolo[i].setVisible(false);
                slBandGain[i].setVisible(false);
            }
        }
        resized();
    }
}

void PolarDesignerAudioProcessorEditor::sliderValueChanged(Slider* slider)
{
    if (slider == &trimSlider) {
        return;
    }
    else
        if (slider == &slCrossoverPosition[0] || slider == &slCrossoverPosition[1] || slider == &slCrossoverPosition[2] || slider == &slCrossoverPosition[3])
        {
            // xOverSlider
            return;
        }
        else
        {
            // dirSlider
            for (int i = 0; i < 5; i++)
            {
                if (slider == &slDir[i])
                    polarPatternVisualizers[i].setDirWeight(slider->getValue());
            }
        }
    directivityEqualiser.repaint();
}

void PolarDesignerAudioProcessorEditor::loadFile()
{
    FileChooser myChooser ("Select Preset File",
                           processor.getLastDir().exists() ? processor.getLastDir() : File::getSpecialLocation (File::userHomeDirectory),
                           "*.json");
    if (myChooser.browseForFileToOpen())
    {
        loadingFile = true;
        File presetFile (myChooser.getResult());
        processor.setLastDir(presetFile.getParentDirectory());
        Result result = processor.loadPreset (presetFile);
        if (!result.wasOk()) {
            errorMessage = result.getErrorMessage();
            alOverlayError.setTitle("preset load error!");
            alOverlayError.setMessage(errorMessage);
            alOverlayError.setVisible(true);
            disableMainArea();
            setSideAreaEnabled(false);
        }
        else
        {
            setEqMode();
        }
        loadingFile = false;
    }
}

void PolarDesignerAudioProcessorEditor::saveFile()
{
    FileChooser myChooser ("Save Preset File",
                           processor.getLastDir().exists() ? processor.getLastDir() : File::getSpecialLocation (File::userHomeDirectory),
                           "*.json");
    if (myChooser.browseForFileToSave (true))
    {
        File presetFile (myChooser.getResult());
        processor.setLastDir(presetFile.getParentDirectory());
        Result result = processor.savePreset (presetFile);
        if (!result.wasOk()) {
            errorMessage = result.getErrorMessage();
            alOverlayError.setTitle("preset save error!");
            alOverlayError.setMessage(errorMessage);
            alOverlayError.setVisible(true);
            disableMainArea();
            setSideAreaEnabled(false);
        }
    }
}

void PolarDesignerAudioProcessorEditor::nActiveBandsChanged()
{
    nActiveBands = processor.getNBands();
    for (int i = 0; i < 5; i++)
    {
        if (i < nActiveBands)
        {
            slDir[i].setEnabled(true);
            slBandGain[i].setEnabled(true);
            msbSolo[i].setEnabled(true);
            msbMute[i].setEnabled(true);
            polarPatternVisualizers[i].setActive(true);
            
            polarPatternVisualizers[i].setVisible(true);
            
            slDir[i].setVisible(true);
            slBandGain[i].setVisible(true);
            msbSolo[i].setVisible(true);
            msbMute[i].setVisible(true);
            
        }
        else
        {
            slDir[i].setEnabled(false);
            slBandGain[i].setEnabled(false);
            msbSolo[i].setEnabled(false);
            msbSolo[i].setToggleState(false, NotificationType::sendNotification);
            msbMute[i].setEnabled(false);
            msbMute[i].setToggleState(false, NotificationType::sendNotification);
            polarPatternVisualizers[i].setActive(false);
            polarPatternVisualizers[i].setVisible(false);
            
            slDir[i].setVisible(false);
            slBandGain[i].setVisible(false);
            msbSolo[i].setVisible(false);
            msbMute[i].setVisible(false);
            

        }
        // sync Channel
        if (i <= nActiveBands) {
            tbSyncChannel[i].setEnabled(true);
        }
        else {
            tbSyncChannel[i].setEnabled(false);

        }
    }
    
    tbSyncChannel[0].setToggleState(true,  NotificationType::sendNotification);
    
    directivityEqualiser.resetTooltipTexts();
    directivityEqualiser.repaint();
    
}

void PolarDesignerAudioProcessorEditor::timerCallback()
{
    if (processor.repaintDEQ.get())
    {
        processor.repaintDEQ = false;
        directivityEqualiser.repaint();
    }
    if (processor.didNRActiveBandsChange.get())
    {
        processor.didNRActiveBandsChange = false;
        nActiveBandsChanged();
    }
    if (processor.zeroDelayModeChanged.get())
    {
        processor.zeroDelayModeChanged = false;
        zeroDelayModeChange();
    }
    if (processor.ffDfEqChanged.get())
    {
        processor.ffDfEqChanged = false;
        setEqMode();
    }
}

void PolarDesignerAudioProcessorEditor::zeroDelayModeChange()
{
    tbZeroDelay.setToggleState(processor.zeroDelayModeActive(), NotificationType::dontSendNotification);
    
    // !J! TODO: replace cbSetNrBands with tbSetNrBands
    nActiveBands = cbSetNrBands.getSelectedId();
    //    nActiveBands = tbSetNrBands.
    
    int nActive = nActiveBands;
    
    if (processor.zeroDelayModeActive())
        nActive = 1;
    
    setSideAreaEnabled(!processor.zeroDelayModeActive());
    
    for (int i = 0; i < 5; i++)
    {
        if (i < nActive)
        {
            slDir[i].setEnabled(true);
            slBandGain[i].setEnabled(true);
            msbSolo[i].setEnabled(true);
            msbMute[i].setEnabled(true);
            polarPatternVisualizers[i].setActive(true);
        }
        else
        {
            slDir[i].setEnabled(false);
            slBandGain[i].setEnabled(false);
            msbSolo[i].setEnabled(false);
            msbSolo[i].setToggleState(false, NotificationType::sendNotification);
            msbMute[i].setEnabled(false);
            msbMute[i].setToggleState(false, NotificationType::sendNotification);
            polarPatternVisualizers[i].setActive(false);
        }
    }
    
    directivityEqualiser.resetTooltipTexts();
    directivityEqualiser.repaint();
}

void PolarDesignerAudioProcessorEditor::disableMainArea()
{
    directivityEqualiser.setActive(false);
    for (int i = 0; i < nActiveBands; i++)
    {
        slDir[i].setEnabled(false);
        slBandGain[i].setEnabled(false);
        msbSolo[i].setEnabled(false);
        msbMute[i].setEnabled(false);
        polarPatternVisualizers[i].setActive(false);
    }
    tbZeroDelay.setEnabled(false);
}

void PolarDesignerAudioProcessorEditor::onAlOverlayErrorOkay()
{
    disableOverlay();
}

void PolarDesignerAudioProcessorEditor::onAlOverlayApplyPattern()
{
    disableOverlay();
    processor.stopTracking(1);
}

void PolarDesignerAudioProcessorEditor::onAlOverlayCancelRecord()
{
    disableOverlay();
    processor.stopTracking(0);
}

void PolarDesignerAudioProcessorEditor::onAlOverlayMaxSigToDist()
{
    disableOverlay();
    processor.stopTracking(2);
}

void PolarDesignerAudioProcessorEditor::setSideAreaEnabled(bool set)
{
    tbSetNrBands[0].setEnabled(set);
    tbSetNrBands[1].setEnabled(set);
    tbSetNrBands[2].setEnabled(set);
    tbSetNrBands[3].setEnabled(set);
    tbSetNrBands[4].setEnabled(set);
    tbSyncChannel[0].setEnabled(set);
    tbSyncChannel[1].setEnabled(set);
    tbSyncChannel[2].setEnabled(set);
    tbSyncChannel[3].setEnabled(set);
    tbSyncChannel[4].setEnabled(set);
    tbSyncChannel[5].setEnabled(set);
    
    //    cbSetNrBands.setEnabled(set);
    //    cbSyncChannel.setEnabled(set);
    tbLoadFile.setEnabled(set);
    tbSaveFile.setEnabled(set);
    tbEq[0].setEnabled(set);
    tbEq[1].setEnabled(set);
    tbEq[2].setEnabled(set);
    tbAllowBackwardsPattern.setEnabled(set);
    tbRecordDisturber.setEnabled(set);
    tbRecordSignal.setEnabled(set);
    slProximity.setEnabled(set);
}

void PolarDesignerAudioProcessorEditor::setEqMode()
{
    int activeIdx = processor.getEqState();
    tbEq[activeIdx].setToggleState(true, NotificationType::sendNotification);
}


void PolarDesignerAudioProcessorEditor::disableOverlay()
{
    alOverlayError.setVisible(false);
    alOverlayDisturber.setVisible(false);
    alOverlaySignal.setVisible(false);
    directivityEqualiser.setActive(true);
    nActiveBandsChanged();
    setSideAreaEnabled(true);
    tbZeroDelay.setEnabled(true);
}

// implement this for AAX automation shortchut
int PolarDesignerAudioProcessorEditor::getControlParameterIndex (Component& control)
{
    if (&control == &directivityEqualiser.getBandlimitPathComponent(0) && nActiveBands > 1)
        return 0;
    else if (&control == &directivityEqualiser.getBandlimitPathComponent(1) && nActiveBands > 2)
        return 1;
    else if (&control == &directivityEqualiser.getBandlimitPathComponent(2) && nActiveBands > 3)
        return 2;
    else if (&control == &directivityEqualiser.getBandlimitPathComponent(3) && nActiveBands > 4)
        return 3;
    else if (&control == &slDir[0] || &control == &directivityEqualiser.getDirPathComponent(0))
        return 4;
    else if ((&control == &slDir[1] || &control == &directivityEqualiser.getDirPathComponent(1)) && nActiveBands > 1)
        return 5;
    else if ((&control == &slDir[2] || &control == &directivityEqualiser.getDirPathComponent(2)) && nActiveBands > 2)
        return 6;
    else if ((&control == &slDir[3] || &control == &directivityEqualiser.getDirPathComponent(3)) && nActiveBands > 3)
        return 7;
    else if ((&control == &slDir[4] || &control == &directivityEqualiser.getDirPathComponent(4)) && nActiveBands > 4)
        return 8;
    else if (&control == &msbSolo[0])
        return 9;
    else if (&control == &msbSolo[1] && nActiveBands > 1)
        return 10;
    else if (&control == &msbSolo[2] && nActiveBands > 2)
        return 11;
    else if (&control == &msbSolo[3] && nActiveBands > 3)
        return 12;
    else if (&control == &msbSolo[4] && nActiveBands > 4)
        return 13;
    else if (&control == &msbMute[0])
        return 14;
    else if (&control == &msbMute[1] && nActiveBands > 1)
        return 15;
    else if (&control == &msbMute[2] && nActiveBands > 2)
        return 16;
    else if (&control == &msbMute[3] && nActiveBands > 3)
        return 17;
    else if (&control == &msbMute[4] && nActiveBands > 4)
        return 18;
    else if (&control == &slBandGain[0])
        return 19;
    else if (&control == &slBandGain[1] && nActiveBands > 1)
        return 20;
    else if (&control == &slBandGain[2] && nActiveBands > 2)
        return 21;
    else if (&control == &slBandGain[3] && nActiveBands > 3)
        return 22;
    else if (&control == &slBandGain[4] && nActiveBands > 4)
        return 23;
    else if (&control == &slProximity)
        return 26;
    
    return -1;
}

